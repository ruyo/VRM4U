// VRM4U Copyright (c) 2021-2026 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertRig.h"
#include "VrmConvert.h"
#include "VrmUtil.h"
#include "VRM4ULoaderLog.h"

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmBPFunctionLibrary.h"

#include "Engine/SkeletalMesh.h"

#include "Animation/MorphTarget.h"
#include "Animation/NodeMappingContainer.h"
#include "Animation/PoseAsset.h"
#include "Animation/Skeleton.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "CommonFrameRates.h"
#if UE_VERSION_OLDER_THAN(5,4,0)
#include "Animation/Rig.h"
#endif


#if WITH_EDITOR
#include "IPersonaToolkit.h"
#include "PersonaModule.h"
#include "Modules/ModuleManager.h"
#include "Animation/DebugSkelMeshComponent.h"
#if UE_VERSION_OLDER_THAN(5,0,0)
#else
#include "Rigs/RigHierarchy.h"
#endif
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>

//#include "Engine/.h"

#if WITH_EDITOR

namespace {
	TSharedRef<IPersonaToolkit> LocalCreatePersonaToolkit(USkeletalMesh* sk) {

		FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>("Persona");

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
		bool b = GIsGameThreadIdInitialized;
		//GIsGameThreadIdInitialized = false;
#endif
#if	UE_VERSION_OLDER_THAN(5,1,0)
		auto PersonaToolkit = PersonaModule.CreatePersonaToolkit(sk);
#else

		static auto f = [](const TSharedRef<IPersonaPreviewScene>& InPersonaPreviewScene) {
		};

		FPersonaToolkitArgs args;
		args.OnPreviewSceneCreated = FOnPreviewSceneCreated::FDelegate::CreateStatic(f);
		//args.OnPreviewSceneCreated = FOnPreviewSceneCreated::FDelegate::CreateSP(this, &FControlRigEditor::HandlePreviewSceneCreated);
		auto PersonaToolkit = PersonaModule.CreatePersonaToolkit(sk, args);
#endif

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
		//GIsGameThreadIdInitialized = b;
#endif
		return PersonaToolkit;
	}

	class AutoDestroy {
	public:
#if	UE_VERSION_OLDER_THAN(5,0,0)
		AActor *actor = nullptr;
#else
		TObjectPtr<AActor> actor;
#endif
		AutoDestroy(AActor* a) {
			actor = a;
		}
		~AutoDestroy() {
			if (actor) {
				actor->Destroy();
			}
		}
	};
}

#endif

namespace {
// utility function 
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,3,0)
	FSmartName GetUniquePoseName(USkeleton* Skeleton, const FString &Name, bool bFind = false)
	{
		if (Skeleton == nullptr) {
			return FSmartName();
		}

		if (bFind) {
			auto NewUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, *Name);
			if (NewUID != SmartName::MaxUID) {
				return FSmartName(*Name, NewUID);
			}
		}

		int32 NameIndex = 0;

		SmartName::UID_Type NewUID;
		FName NewName;

		do
		{
			NewName = FName(*FString::Printf(TEXT("%s_%d"), *Name,NameIndex++));

			if (NameIndex == 1) {
				NewName = *Name;
			}

			NewUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, NewName);
		} while (NewUID != SmartName::MaxUID);

		// if found, 
		FSmartName NewPoseName;
		Skeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, NewName, NewPoseName);

		return NewPoseName;
	}
#else
	FName GetUniquePoseName(USkeleton* Skeleton, const FString& Name, bool bFind = false)
	{
		if (Skeleton == nullptr) {
			return "";
		}

		if (bFind) {
			bool bSame = false;
			Skeleton->ForEachCurveMetaData([Name, &bSame](FName InCurveName, const FCurveMetaData& InMetaData)
			{
					if (InCurveName.ToString().ToLower() == Name.ToLower()) {
						bSame = true;

					}
			});
			if (bSame) {
				return *Name;
			}
		}

		int32 NameIndex = 0;
		FName NewName;

		do
		{
			NewName = FName(*FString::Printf(TEXT("%s_%d"), *Name, NameIndex++));

			if (NameIndex == 1) {
				NewName = *Name;
			}
		} while (Skeleton->AddCurveMetaData(NewName) == false);


		return NewName;
	}
#endif

#endif 

#if WITH_EDITOR
	struct FPerfectSyncMorphMapping {
		FString PoseName;
		FString MorphTargetName;
	};

	struct FPerfectSyncFallbackMapping {
		FString PerfectSyncPoseName;
		FString VrmPoseName;
	};

	struct FMetaHumanCurveContribution {
		FString MetaHumanCurveName;
		FString PerfectSyncPoseName;
		float Weight;
	};

	struct FPreviewMorphCurve {
		FString MorphTargetName;
		TMap<int32, float> ValuesByPose;
	};

	struct FMetaHumanPoseFrame {
		FString CurveName;
		int32 PoseIndex;
	};

	static FString FindSkeletalMeshMorphTargetName(const USkeletalMesh* SkeletalMesh, const FString& MorphTargetName) {
		if (SkeletalMesh == nullptr || MorphTargetName.IsEmpty()) {
			return FString();
		}

		for (const auto& MorphTarget : SkeletalMesh->GetMorphTargets()) {
			if (MorphTarget && MorphTarget->GetName().Equals(MorphTargetName, ESearchCase::IgnoreCase)) {
				return MorphTarget->GetName();
			}
		}

		return FString();
	}

	static void SetPreviewMorphValue(TArray<FPreviewMorphCurve>& PreviewMorphCurves, const FString& MorphTargetName, int32 PoseIndex, float Value) {
		if (MorphTargetName.IsEmpty() || PoseIndex < 0) {
			return;
		}

		FPreviewMorphCurve* PreviewCurve = PreviewMorphCurves.FindByPredicate(
			[&MorphTargetName](const FPreviewMorphCurve& Curve) {
				return Curve.MorphTargetName.Equals(MorphTargetName, ESearchCase::IgnoreCase);
			});
		if (PreviewCurve == nullptr) {
			PreviewCurve = &PreviewMorphCurves.AddDefaulted_GetRef();
			PreviewCurve->MorphTargetName = MorphTargetName;
		}

		PreviewCurve->ValuesByPose.Add(PoseIndex, Value);
	}

	static void AddMetaHumanMarkerCurves(
		UAnimSequence* Animation,
		USkeleton* Skeleton,
		const TArray<FMetaHumanPoseFrame>& MetaHumanPoseFrames,
		int32 PoseCount) {
		if (Animation == nullptr || Skeleton == nullptr || MetaHumanPoseFrames.Num() == 0 || PoseCount <= 0) {
			return;
		}

#if UE_VERSION_OLDER_THAN(5,2,0)
#if UE_VERSION_OLDER_THAN(5,0,0)
#else
		IAnimationDataController& MarkerDataController = Animation->GetController();
		IAnimationDataController::FScopedBracket MarkerScopedBracket(&MarkerDataController, FText());
#endif
		for (const FMetaHumanPoseFrame& PoseFrame : MetaHumanPoseFrames) {
			auto CurveName = GetUniquePoseName(Skeleton, PoseFrame.CurveName, true);
			Animation->RawCurveData.AddCurveData(CurveName);
			FFloatCurve* MarkerCurve = Animation->RawCurveData.FloatCurves.FindByPredicate(
				[&CurveName](const FFloatCurve& Curve) {
					return Curve.Name.DisplayName == CurveName.DisplayName;
				});
			if (MarkerCurve == nullptr) {
				continue;
			}

			MarkerCurve->SetCurveTypeFlag(AACF_Editable, true);
			for (int32 FrameIndex = 0; FrameIndex < PoseCount; ++FrameIndex) {
				MarkerCurve->UpdateOrAddKey(FrameIndex == PoseFrame.PoseIndex ? 1.f : 0.f, static_cast<float>(FrameIndex));
			}

#if UE_VERSION_OLDER_THAN(5,0,0)
#else
			const FAnimationCurveIdentifier CurveId(CurveName, ERawCurveTrackTypes::RCT_Float);
			MarkerDataController.AddCurve(CurveId);
			MarkerDataController.SetCurveKeys(CurveId, MarkerCurve->FloatCurve.GetConstRefOfKeys());
#endif
		}
#if UE_VERSION_OLDER_THAN(5,0,0)
#else
		MarkerDataController.UpdateCurveNamesFromSkeleton(Skeleton, ERawCurveTrackTypes::RCT_Float);
		MarkerDataController.NotifyPopulated();
#endif
#else
		IAnimationDataController& MarkerDataController = Animation->GetController();
		IAnimationDataController::FScopedBracket MarkerScopedBracket(&MarkerDataController, FText());
		for (const FMetaHumanPoseFrame& PoseFrame : MetaHumanPoseFrames) {
			const auto CurveName = GetUniquePoseName(Skeleton, PoseFrame.CurveName, true);
			const FAnimationCurveIdentifier CurveId(CurveName, ERawCurveTrackTypes::RCT_Float);
			MarkerDataController.AddCurve(CurveId);
			MarkerDataController.SetCurveFlag(CurveId, AACF_Editable, true);

			TArray<FRichCurveKey> CurveKeys;
			CurveKeys.Reserve(PoseCount);
			for (int32 FrameIndex = 0; FrameIndex < PoseCount; ++FrameIndex) {
				FRichCurveKey& Key = CurveKeys.Emplace_GetRef(
					static_cast<float>(FrameIndex),
					FrameIndex == PoseFrame.PoseIndex ? 1.f : 0.f);
				Key.InterpMode = RCIM_Constant;
			}
			MarkerDataController.SetCurveKeys(CurveId, CurveKeys);
		}
#if UE_VERSION_OLDER_THAN(5,3,0)
		MarkerDataController.UpdateCurveNamesFromSkeleton(Skeleton, ERawCurveTrackTypes::RCT_Float);
#endif
		MarkerDataController.NotifyPopulated();
#endif
	}

	static UAnimSequence* CreateFacePreviewAnimation(
		UVrmAssetListObject* VrmAssetList,
		USkeletalMesh* SkeletalMesh,
		USkeleton* Skeleton,
		const TArray<FPreviewMorphCurve>& PreviewMorphCurves,
		int32 PoseCount) {
		if (VrmAssetList == nullptr || SkeletalMesh == nullptr || Skeleton == nullptr || PoseCount <= 0) {
			return nullptr;
		}

		const FString PreviewAnimName = FString(TEXT("A_face_preview_")) + VrmAssetList->BaseFileName;
		UAnimSequence* PreviewAnimation = VRM4U_NewObject<UAnimSequence>(
			VrmAssetList->Package,
			*PreviewAnimName,
			RF_Public | RF_Standalone);
		PreviewAnimation->SetSkeleton(Skeleton);
		PreviewAnimation->RateScale = 24.f;

#if UE_VERSION_OLDER_THAN(5,0,0)
		PreviewAnimation->CleanAnimSequenceForImport();
#elif UE_VERSION_OLDER_THAN(5,2,0)
		IAnimationDataController& PreviewDataController = PreviewAnimation->GetController();
		IAnimationDataController::FScopedBracket PreviewScopedBracket(&PreviewDataController, FText());
		PreviewDataController.ResetModel();
#else
		IAnimationDataController& PreviewDataController = PreviewAnimation->GetController();
		IAnimationDataController::FScopedBracket PreviewScopedBracket(&PreviewDataController, FText());
		PreviewDataController.ResetModel();
		PreviewDataController.InitializeModel();
		PreviewDataController.NotifyPopulated();
		PreviewDataController.SetFrameRate(FFrameRate(1, 1));
		PreviewDataController.SetNumberOfFrames(PoseCount - 1);
		PreviewDataController.UpdateWithSkeleton(Skeleton);
#endif

		for (const FPreviewMorphCurve& PreviewMorphCurve : PreviewMorphCurves) {
			auto CurveName = GetUniquePoseName(Skeleton, PreviewMorphCurve.MorphTargetName, true);

#if UE_VERSION_OLDER_THAN(5,2,0)
			PreviewAnimation->RawCurveData.AddCurveData(CurveName);
			FFloatCurve& Curve = PreviewAnimation->RawCurveData.FloatCurves.Last();
			Curve.SetCurveTypeFlag(AACF_Editable, true);
#if UE_VERSION_OLDER_THAN(5,0,0)
			Skeleton->AccumulateCurveMetaData(CurveName.DisplayName, false, true);
#elif UE_VERSION_OLDER_THAN(5,3,0)
			Skeleton->AccumulateCurveMetaData(CurveName.DisplayName, false, true);
#else
			Skeleton->SetCurveMetaDataMorphTarget(CurveName.DisplayName, true);
#endif
			for (int32 PoseIndex = 0; PoseIndex < PoseCount; ++PoseIndex) {
				const float* Value = PreviewMorphCurve.ValuesByPose.Find(PoseIndex);
				Curve.UpdateOrAddKey(Value ? *Value : 0.f, static_cast<float>(PoseIndex));
			}
#else

#if UE_VERSION_OLDER_THAN(5,3,0)
			Skeleton->AccumulateCurveMetaData(CurveName.DisplayName, false, true);
#elif UE_VERSION_OLDER_THAN(5,7,0)
			Skeleton->AddCurveMetaData(CurveName);
			if (UAnimCurveMetaData* CurveMetaData =
				Skeleton->GetAssetUserData<UAnimCurveMetaData>())
			{
				CurveMetaData->SetCurveMetaDataMorphTarget(CurveName, true);
			}
#else
			Skeleton->SetCurveMetaDataMorphTarget(CurveName, true);
#endif

			const FAnimationCurveIdentifier CurveId(CurveName, ERawCurveTrackTypes::RCT_Float);
			PreviewDataController.AddCurve(CurveId);
			PreviewDataController.SetCurveFlag(CurveId, AACF_Editable, true);

			TArray<FRichCurveKey> CurveKeys;
			CurveKeys.Reserve(PoseCount);
			for (int32 PoseIndex = 0; PoseIndex < PoseCount; ++PoseIndex) {
				const float* Value = PreviewMorphCurve.ValuesByPose.Find(PoseIndex);
				FRichCurveKey& Key = CurveKeys.Emplace_GetRef(static_cast<float>(PoseIndex), Value ? *Value : 0.f);
				Key.InterpMode = RCIM_Constant;
			}
			PreviewDataController.SetCurveKeys(CurveId, CurveKeys);
#endif
		}

#if UE_VERSION_OLDER_THAN(5,0,0)
		PreviewAnimation->SetRawNumberOfFrame(PoseCount);
#endif

#if UE_VERSION_OLDER_THAN(5,0,0)
		PreviewAnimation->SequenceLength = static_cast<float>(PoseCount - 1);
#elif UE_VERSION_OLDER_THAN(5,2,0)
		PreviewDataController.SetPlayLength(static_cast<float>(PoseCount - 1));
		PreviewDataController.SetFrameRate(FFrameRate(1, 1));
		PreviewDataController.UpdateCurveNamesFromSkeleton(Skeleton, ERawCurveTrackTypes::RCT_Float);
		PreviewDataController.NotifyPopulated();
#else
		PreviewAnimation->SetPreviewMesh(SkeletalMesh);
		PreviewDataController.NotifyPopulated();
#endif

#if UE_VERSION_OLDER_THAN(5,0,0)
		PreviewAnimation->PreSave(nullptr);
#endif
		PreviewAnimation->PostEditChange();

		return PreviewAnimation;
	}

	static const TArray<FString>& GetPerfectSyncPoseNames() {
		static const TArray<FString> PoseNames = {
			// Left eye blend shapes
			TEXT("EyeBlinkLeft"),
			TEXT("EyeLookDownLeft"),
			TEXT("EyeLookInLeft"),
			TEXT("EyeLookOutLeft"),
			TEXT("EyeLookUpLeft"),
			TEXT("EyeSquintLeft"),
			TEXT("EyeWideLeft"),
			// Right eye blend shapes
			TEXT("EyeBlinkRight"),
			TEXT("EyeLookDownRight"),
			TEXT("EyeLookInRight"),
			TEXT("EyeLookOutRight"),
			TEXT("EyeLookUpRight"),
			TEXT("EyeSquintRight"),
			TEXT("EyeWideRight"),
			// Jaw blend shapes
			TEXT("JawForward"),
			TEXT("JawLeft"),
			TEXT("JawRight"),
			TEXT("JawOpen"),
			// Mouth blend shapes
			TEXT("MouthClose"),
			TEXT("MouthFunnel"),
			TEXT("MouthPucker"),
			TEXT("MouthLeft"),
			TEXT("MouthRight"),
			TEXT("MouthSmileLeft"),
			TEXT("MouthSmileRight"),
			TEXT("MouthFrownLeft"),
			TEXT("MouthFrownRight"),
			TEXT("MouthDimpleLeft"),
			TEXT("MouthDimpleRight"),
			TEXT("MouthStretchLeft"),
			TEXT("MouthStretchRight"),
			TEXT("MouthRollLower"),
			TEXT("MouthRollUpper"),
			TEXT("MouthShrugLower"),
			TEXT("MouthShrugUpper"),
			TEXT("MouthPressLeft"),
			TEXT("MouthPressRight"),
			TEXT("MouthLowerDownLeft"),
			TEXT("MouthLowerDownRight"),
			TEXT("MouthUpperUpLeft"),
			TEXT("MouthUpperUpRight"),
			// Brow blend shapes
			TEXT("BrowDownLeft"),
			TEXT("BrowDownRight"),
			TEXT("BrowInnerUp"),
			TEXT("BrowOuterUpLeft"),
			TEXT("BrowOuterUpRight"),
			// Cheek blend shapes
			TEXT("CheekPuff"),
			TEXT("CheekSquintLeft"),
			TEXT("CheekSquintRight"),
			// Nose blend shapes
			TEXT("NoseSneerLeft"),
			TEXT("NoseSneerRight"),
			TEXT("TongueOut"),
			// Treat the head rotation as curves for LiveLink support
			TEXT("HeadYaw"),
			TEXT("HeadPitch"),
			TEXT("HeadRoll"),
			// Treat eye rotation as curves for LiveLink support
			TEXT("LeftEyeYaw"),
			TEXT("LeftEyePitch"),
			TEXT("LeftEyeRoll"),
			TEXT("RightEyeYaw"),
			TEXT("RightEyePitch"),
			TEXT("RightEyeRoll"),
		};
		return PoseNames;
	}

	static FString FindPerfectSyncMorphTarget(const FString& PerfectSyncPoseName, const TArray<FString>& MorphNameList) {
		const FString* ExactMatch = MorphNameList.FindByPredicate(
			[&PerfectSyncPoseName](const FString& MorphName) {
				return MorphName.Equals(PerfectSyncPoseName, ESearchCase::CaseSensitive);
			});
		if (ExactMatch) {
			return *ExactMatch;
		}

		const FString* CaseInsensitiveMatch = MorphNameList.FindByPredicate(
			[&PerfectSyncPoseName](const FString& MorphName) {
				return MorphName.Equals(PerfectSyncPoseName, ESearchCase::IgnoreCase);
			});
		if (CaseInsensitiveMatch) {
			return *CaseInsensitiveMatch;
		}

		TArray<FString> AliasNames;
		AliasNames.Add(PerfectSyncPoseName.Replace(TEXT("Left"), TEXT("_L"), ESearchCase::CaseSensitive));
		AliasNames.Add(PerfectSyncPoseName.Replace(TEXT("Right"), TEXT("_R"), ESearchCase::CaseSensitive));

		for (const FString& AliasName : AliasNames) {
			if (AliasName == PerfectSyncPoseName) {
				continue;
			}
			const FString* AliasMatch = MorphNameList.FindByPredicate(
				[&AliasName](const FString& MorphName) {
					return MorphName.Equals(AliasName, ESearchCase::IgnoreCase);
				});
			if (AliasMatch) {
				return *AliasMatch;
			}
		}

		return FString();
	}

	static TArray<FPerfectSyncMorphMapping> BuildPerfectSyncMorphMappings(const TArray<FString>& MorphNameList) {
		TArray<FPerfectSyncMorphMapping> Mappings;
		const TArray<FString>& PerfectSyncPoseNames = GetPerfectSyncPoseNames();
		Mappings.Reserve(PerfectSyncPoseNames.Num());

		for (const FString& PoseName : PerfectSyncPoseNames) {
			FPerfectSyncMorphMapping Mapping;
			Mapping.PoseName = PoseName;
			Mapping.MorphTargetName = FindPerfectSyncMorphTarget(PoseName, MorphNameList);
			Mappings.Add(Mapping);
		}

		return Mappings;
	}

	static const TArray<FPerfectSyncFallbackMapping>& GetPerfectSyncFallbackMappings() {
		static const TArray<FPerfectSyncFallbackMapping> Mappings = {
			{TEXT("EyeBlinkLeft"), TEXT("Blink_L")},
			{TEXT("EyeBlinkRight"), TEXT("Blink_R")},
			{TEXT("JawOpen"), TEXT("A")},
		};
		return Mappings;
	}

	static const TArray<FMetaHumanCurveContribution>& GetMetaHumanCurveContributions() {
		// Keep these normalized semantic contributions local so generated PoseAssets do not depend on MetaHuman plugins or assets.
		static const TArray<FMetaHumanCurveContribution> Contributions = {
			{TEXT("CTRL_expressions_eyeBlinkL"), TEXT("EyeBlinkLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeLookDownL"), TEXT("EyeLookDownLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeLookRightL"), TEXT("EyeLookInLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeLookLeftL"), TEXT("EyeLookOutLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeLookUpL"), TEXT("EyeLookUpLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeSquintInnerL"), TEXT("EyeSquintLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeWidenL"), TEXT("EyeWideLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeBlinkR"), TEXT("EyeBlinkRight"), 1.f},
			{TEXT("CTRL_expressions_eyeLookDownR"), TEXT("EyeLookDownRight"), 1.f},
			{TEXT("CTRL_expressions_eyeLookLeftR"), TEXT("EyeLookInRight"), 1.f},
			{TEXT("CTRL_expressions_eyeLookRightR"), TEXT("EyeLookOutRight"), 1.f},
			{TEXT("CTRL_expressions_eyeLookUpR"), TEXT("EyeLookUpRight"), 1.f},
			{TEXT("CTRL_expressions_eyeSquintInnerR"), TEXT("EyeSquintRight"), 1.f},
			{TEXT("CTRL_expressions_eyeWidenR"), TEXT("EyeWideRight"), 1.f},
			{TEXT("CTRL_expressions_jawFwd"), TEXT("JawForward"), 1.f},
			{TEXT("CTRL_expressions_jawLeft"), TEXT("JawLeft"), 1.f},
			{TEXT("CTRL_expressions_jawRight"), TEXT("JawRight"), 1.f},
			{TEXT("CTRL_expressions_jawOpen"), TEXT("JawOpen"), 1.f},
			{TEXT("CTRL_expressions_mouthLipsTogetherUL"), TEXT("MouthClose"), 0.25f},
			{TEXT("CTRL_expressions_mouthLipsTogetherUR"), TEXT("MouthClose"), 0.25f},
			{TEXT("CTRL_expressions_mouthLipsTogetherDL"), TEXT("MouthClose"), 0.25f},
			{TEXT("CTRL_expressions_mouthLipsTogetherDR"), TEXT("MouthClose"), 0.25f},
			{TEXT("CTRL_expressions_mouthFunnelUL"), TEXT("MouthFunnel"), 0.25f},
			{TEXT("CTRL_expressions_mouthFunnelUR"), TEXT("MouthFunnel"), 0.25f},
			{TEXT("CTRL_expressions_mouthFunnelDL"), TEXT("MouthFunnel"), 0.25f},
			{TEXT("CTRL_expressions_mouthFunnelDR"), TEXT("MouthFunnel"), 0.25f},
			{TEXT("CTRL_expressions_mouthLipsPurseUL"), TEXT("MouthPucker"), 0.25f},
			{TEXT("CTRL_expressions_mouthLipsPurseUR"), TEXT("MouthPucker"), 0.25f},
			{TEXT("CTRL_expressions_mouthLipsPurseDL"), TEXT("MouthPucker"), 0.25f},
			{TEXT("CTRL_expressions_mouthLipsPurseDR"), TEXT("MouthPucker"), 0.25f},
			{TEXT("CTRL_expressions_mouthLeft"), TEXT("MouthLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthRight"), TEXT("MouthRight"), 1.f},
			{TEXT("CTRL_expressions_mouthCornerPullL"), TEXT("MouthSmileLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthCornerPullR"), TEXT("MouthSmileRight"), 1.f},
			{TEXT("CTRL_expressions_mouthCornerDepressL"), TEXT("MouthFrownLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthCornerDepressR"), TEXT("MouthFrownRight"), 1.f},
			{TEXT("CTRL_expressions_mouthDimpleL"), TEXT("MouthDimpleLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthDimpleR"), TEXT("MouthDimpleRight"), 1.f},
			{TEXT("CTRL_expressions_mouthStretchL"), TEXT("MouthStretchLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthStretchR"), TEXT("MouthStretchRight"), 1.f},
			{TEXT("CTRL_expressions_mouthLowerLipRollInL"), TEXT("MouthRollLower"), 0.5f},
			{TEXT("CTRL_expressions_mouthLowerLipRollInR"), TEXT("MouthRollLower"), 0.5f},
			{TEXT("CTRL_expressions_mouthUpperLipRollInL"), TEXT("MouthRollUpper"), 0.5f},
			{TEXT("CTRL_expressions_mouthUpperLipRollInR"), TEXT("MouthRollUpper"), 0.5f},
			{TEXT("CTRL_expressions_mouthLowerLipTowardsTeethL"), TEXT("MouthShrugLower"), 0.5f},
			{TEXT("CTRL_expressions_mouthLowerLipTowardsTeethR"), TEXT("MouthShrugLower"), 0.5f},
			{TEXT("CTRL_expressions_mouthUpperLipTowardsTeethL"), TEXT("MouthShrugUpper"), 0.5f},
			{TEXT("CTRL_expressions_mouthUpperLipTowardsTeethR"), TEXT("MouthShrugUpper"), 0.5f},
			{TEXT("CTRL_expressions_mouthLipsPressL"), TEXT("MouthPressLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthLipsPressR"), TEXT("MouthPressRight"), 1.f},
			{TEXT("CTRL_expressions_mouthLowerLipDepressL"), TEXT("MouthLowerDownLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthLowerLipDepressR"), TEXT("MouthLowerDownRight"), 1.f},
			{TEXT("CTRL_expressions_mouthUpperLipRaiseL"), TEXT("MouthUpperUpLeft"), 1.f},
			{TEXT("CTRL_expressions_mouthUpperLipRaiseR"), TEXT("MouthUpperUpRight"), 1.f},
			{TEXT("CTRL_expressions_browDownL"), TEXT("BrowDownLeft"), 1.f},
			{TEXT("CTRL_expressions_browDownR"), TEXT("BrowDownRight"), 1.f},
			{TEXT("CTRL_expressions_browRaiseInL"), TEXT("BrowInnerUp"), 0.5f},
			{TEXT("CTRL_expressions_browRaiseInR"), TEXT("BrowInnerUp"), 0.5f},
			{TEXT("CTRL_expressions_browRaiseOuterL"), TEXT("BrowOuterUpLeft"), 1.f},
			{TEXT("CTRL_expressions_browRaiseOuterR"), TEXT("BrowOuterUpRight"), 1.f},
			{TEXT("CTRL_expressions_mouthCheekBlowL"), TEXT("CheekPuff"), 0.5f},
			{TEXT("CTRL_expressions_mouthCheekBlowR"), TEXT("CheekPuff"), 0.5f},
			{TEXT("CTRL_expressions_eyeCheekRaiseL"), TEXT("CheekSquintLeft"), 1.f},
			{TEXT("CTRL_expressions_eyeCheekRaiseR"), TEXT("CheekSquintRight"), 1.f},
			{TEXT("CTRL_expressions_noseWrinkleL"), TEXT("NoseSneerLeft"), 1.f},
			{TEXT("CTRL_expressions_noseWrinkleR"), TEXT("NoseSneerRight"), 1.f},
			{TEXT("CTRL_expressions_tongueOut"), TEXT("TongueOut"), 1.f},
		};
		return Contributions;
	}

#if UE_VERSION_OLDER_THAN(5,3,0)
	static int32 FindPoseIndexByName(const TArray<FSmartName>& PoseNames, const FString& PoseName) {
		const FSmartName* FoundPoseName = PoseNames.FindByPredicate(
			[&PoseName](const FSmartName& Name) {
				return Name.DisplayName.ToString().Equals(PoseName, ESearchCase::IgnoreCase);
			});
		return FoundPoseName ? PoseNames.Find(*FoundPoseName) : INDEX_NONE;
	}
#else
	static int32 FindPoseIndexByName(const TArray<FName>& PoseNames, const FString& PoseName) {
		const FName* FoundPoseName = PoseNames.FindByPredicate(
			[&PoseName](const FName& Name) {
				return Name.ToString().Equals(PoseName, ESearchCase::IgnoreCase);
			});
		return FoundPoseName ? PoseNames.Find(*FoundPoseName) : INDEX_NONE;
	}
#endif

	static void localFaceMorphConv(UVrmAssetListObject* vrmAssetList, const aiScene* aiData) {

		FString name = FString(TEXT("POSE_face_")) + vrmAssetList->BaseFileName;


		auto &sk = vrmAssetList->SkeletalMesh;
		auto *k = VRMGetSkeleton(sk);

		UPoseAsset* pose = nullptr;
		{
			pose = VRM4U_NewObject<UPoseAsset>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);
			pose->SetSkeleton(k);
			pose->SetPreviewMesh(sk);
			pose->Modify();

			vrmAssetList->PoseFace = pose;
		}

		TArray<FString> MorphNameList;
		{
			for (uint32_t m = 0; m < aiData->mNumMeshes; ++m) {
				const aiMesh& aiM = *(aiData->mMeshes[m]);
				for (uint32_t a = 0; a < aiM.mNumAnimMeshes; ++a) {
					const aiAnimMesh& aiA = *(aiM.mAnimMeshes[a]);
					FString morphName = UTF8_TO_TCHAR(aiA.mName.C_Str());
					MorphNameList.AddUnique(morphName);
				}
			}
		}

		{
#if	UE_VERSION_OLDER_THAN(5,0,0)
			auto PersonaToolkit = LocalCreatePersonaToolkit(sk);
			UDebugSkelMeshComponent* PreviewComponent = PersonaToolkit->GetPreviewMeshComponent();
			auto* skc = Cast<USkeletalMeshComponent>(PreviewComponent);

#elif	UE_VERSION_OLDER_THAN(5,1,0)
			USkeletalMeshComponent* skc = nullptr;
			if (GWorld) {
				ASkeletalMeshActor* ska = GWorld->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FTransform::Identity);
				AutoDestroy autoDestroy(ska);
				skc = Cast<USkeletalMeshComponent>(ska->GetRootComponent());
				skc->SetSkeletalMesh(sk);
			}
#else
			USkeletalMeshComponent* skc = nullptr;
			//if (GWorld) {
				ASkeletalMeshActor* ska = GWorld->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FTransform::Identity);
				AutoDestroy autoDestroy(ska);
				skc = Cast<USkeletalMeshComponent>(ska->GetRootComponent());
				skc->SetSkeletalMeshAsset(sk);
			//}
#endif
			skc->SetComponentSpaceTransformsDoubleBuffering(false);
		}

#if UE_VERSION_OLDER_THAN(5,3,0)
		TArray < FSmartName > SmartNamePoseList;
#else
		TArray < FName > SmartNamePoseList;
#endif
		TArray<FPreviewMorphCurve> PreviewMorphCurves;
		TArray<FMetaHumanPoseFrame> MetaHumanPoseFrames;
		{
			auto n = GetUniquePoseName(k, TEXT("DefaultRefPose"), true);
			SmartNamePoseList.Add(n);
		}

		UAnimSequence* ase = nullptr;
		{
			/*
			auto LocalGetCurveData = [](UAnimSequence* ase) {
#if UE_VERSION_OLDER_THAN(5,2,0)
				return (ase->RawCurveData);
#else
				return ase->GetCurveData();
#endif
			};
			*/


			FString AnimName = FString(TEXT("A_face_")) + vrmAssetList->BaseFileName;
			ase = VRM4U_NewObject<UAnimSequence>(vrmAssetList->Package, *AnimName, RF_Public | RF_Standalone);
			ase->SetSkeleton(k);

#if UE_VERSION_OLDER_THAN(5,0,0)
			ase->CleanAnimSequenceForImport();
#elif UE_VERSION_OLDER_THAN(5,2,0)
			IAnimationDataController& DataController = ase->GetController();
			IAnimationDataController::FScopedBracket ScopedBracket(&DataController, FText());
			DataController.ResetModel();
#else

			IAnimationDataController& DataController = ase->GetController();
			IAnimationDataController::FScopedBracket ScopedBracket(&DataController, FText());

			DataController.ResetModel();
			DataController.InitializeModel();
			DataController.NotifyPopulated();

			FFrameRate f(1, 1);
			DataController.SetFrameRate(f);
			//DataController.SetFrameRate(FCommonFrameRates::FPS_30());
			DataController.SetNumberOfFrames(10);

			DataController.UpdateWithSkeleton(k);
#endif

#if UE_VERSION_OLDER_THAN(5,2,0)
			auto GetCurves = [ase] ()-> TArray<FFloatCurve> &{
				return ase->RawCurveData.FloatCurves;
			};

			auto GetCurveName = [](FFloatCurve& c) {
				return c.Name.DisplayName;
			};
			auto GetCurveSmartName = [](FFloatCurve& c) {
				return c.Name;
			};
#elif UE_VERSION_OLDER_THAN(5,3,0)
			auto GetCurves = [&DataController]() {
				return DataController.GetModel()->GetFloatCurves();
			};

			auto GetCurveName = [](FFloatCurve& c) {
				return c.Name.DisplayName;
			};
			auto GetCurveSmartName = [](FFloatCurve& c) {
				return c.Name;
			};
#else
			auto GetCurves = [&DataController]() {
				return DataController.GetModel()->GetFloatCurves();
			};

			auto GetCurveName = [](FFloatCurve& c) {
				return c.GetName();
			};
			auto GetCurveSmartName = [](FFloatCurve& c) {
				return c.GetName();
			};
#endif
			// A VRM file can define its own custom blend shape clip using the exact
			// same name as a PerfectSync/MetaHuman pose (this is common: VRM authors
			// add ARKit-named clips for VSeeFace/PerfectSync support). If we then add
			// ANOTHER pose under that same name, SmartNamePoseList ends up with a
			// duplicate entry, and UPoseAsset::CreatePoseFromAnimation cannot keep two
			// poses with the same name straight - later poses silently end up reading
			// an earlier, unrelated pose's frame data. Guard against that here.
			auto IsPoseNameAlreadyUsed = [&](const FString& NameToCheck) {
				for (const auto& ExistingName : SmartNamePoseList) {
#if UE_VERSION_OLDER_THAN(5,3,0)
					if (ExistingName.DisplayName.ToString().Equals(NameToCheck, ESearchCase::IgnoreCase)) {
#else
					if (ExistingName.ToString().Equals(NameToCheck, ESearchCase::IgnoreCase)) {
#endif
						return true;
					}
				}
				return false;
			};
			auto AddPerfectSyncPoses = [&]() {
				// Perfect Sync (ARKit) blend shapes and Live Link rotation curves.
				const TArray<FPerfectSyncMorphMapping> PerfectSyncMappings = BuildPerfectSyncMorphMappings(MorphNameList);
				for (const FPerfectSyncMorphMapping& Mapping : PerfectSyncMappings) {
					const FString& PerfectSyncPoseName = Mapping.PoseName;
					const FString& ModelMorphName = Mapping.MorphTargetName;

					if (IsPoseNameAlreadyUsed(PerfectSyncPoseName)) {
						// Already created (e.g. by a same-named VRM blend shape clip) -
						// do not add a second, duplicate-named pose.
						continue;
					}

					const int32 PoseIndex = SmartNamePoseList.Num();
					SetPreviewMorphValue(
						PreviewMorphCurves,
						FindSkeletalMeshMorphTargetName(sk, ModelMorphName),
						PoseIndex,
						1.f);

					auto SmartPoseName = GetUniquePoseName(k, *PerfectSyncPoseName, true);
					auto curveName = GetUniquePoseName(nullptr, "");

					int targetNo = 0;
					bool bSameName = false;
					if (PerfectSyncPoseName.Equals(ModelMorphName, ESearchCase::IgnoreCase)) {
						// same name. no curve weight
						curveName = SmartPoseName;
						bSameName = true;
					} else if (ModelMorphName.IsEmpty()) {
						// no morph. norcurve weight
						curveName = SmartPoseName;
						bSameName = true;
					} else {
						bool bFind = false;
						for (decltype(auto) c : GetCurves()) {
							if (GetCurveName(c).ToString().Equals(ModelMorphName, ESearchCase::IgnoreCase)) {
								// found curve in list
								bFind = true;
								curveName = GetCurveSmartName(c);
								break;
							}
							++targetNo;
						}
						if (bFind == false) {
							// create new curve
							curveName = GetUniquePoseName(k, *ModelMorphName, true);
							targetNo = GetCurves().Num();
						}
					}
#if UE_VERSION_OLDER_THAN(5,2,0)
					ase->RawCurveData.AddCurveData(curveName);
#else
					FAnimationCurveIdentifier id(curveName, ERawCurveTrackTypes::RCT_Float);
					DataController.AddCurve(id);
#endif

					// Anim to Pose
					if (bSameName == false) {
						// Re-resolve the curve index by name instead of trusting the index
						// captured before AddCurve above: the curve array is not guaranteed to
						// simply grow by appending at the previously-observed Num(), which can
						// silently alias this write onto an unrelated existing curve.
						targetNo = 0;
						for (decltype(auto) c2 : GetCurves()) {
							if (GetCurveName(c2).ToString().Equals(ModelMorphName, ESearchCase::IgnoreCase)) {
								break;
							}
							++targetNo;
						}
						decltype(auto) c = GetCurves();
						auto& a = c[targetNo];

						a.SetCurveTypeFlag(AACF_Editable, true);

						// 0 for prev and forward frame
						if (a.Evaluate(PoseIndex - 1) == 0) {
							a.UpdateOrAddKey(0, PoseIndex - 1);
						}
						if (a.Evaluate(PoseIndex + 1) == 0) {
							a.UpdateOrAddKey(0, PoseIndex + 1);
						}
						a.UpdateOrAddKey(1, PoseIndex);

#if UE_VERSION_OLDER_THAN(5,0,0)
#else
						FAnimationCurveIdentifier CurveId(curveName, ERawCurveTrackTypes::RCT_Float);
						DataController.AddCurve(CurveId);
						DataController.SetCurveKeys(CurveId, a.FloatCurve.GetConstRefOfKeys());
#endif
					}
					auto newName = SmartPoseName;
					SmartNamePoseList.Add(newName);
				}
			};

			auto AddMetaHumanPoses = [&]() {
				const TArray<FMetaHumanCurveContribution>& Contributions = GetMetaHumanCurveContributions();
				TArray<FString> MetaHumanCurveNames;
				for (const FMetaHumanCurveContribution& Contribution : Contributions) {
					MetaHumanCurveNames.AddUnique(Contribution.MetaHumanCurveName);
				}

				for (const FString& MetaHumanCurveName : MetaHumanCurveNames) {
					if (IsPoseNameAlreadyUsed(MetaHumanCurveName)) {
						// Already created as a pose under this exact name earlier in this
						// same import pass - do not add a duplicate-named pose.
						continue;
					}

					bool bHasTargetMorph = false;
					for (const FMetaHumanCurveContribution& Contribution : Contributions) {
						if (Contribution.MetaHumanCurveName != MetaHumanCurveName) {
							continue;
						}

						const FString PerfectSyncMorphName = FindPerfectSyncMorphTarget(Contribution.PerfectSyncPoseName, MorphNameList);
						if (FindSkeletalMeshMorphTargetName(sk, PerfectSyncMorphName).IsEmpty() == false) {
							bHasTargetMorph = true;
							break;
						}
					}
					if (bHasTargetMorph == false) {
						continue;
					}

					const int32 PoseIndex = SmartNamePoseList.Num();
					auto SmartPoseName = GetUniquePoseName(k, MetaHumanCurveName, true);

					for (const FMetaHumanCurveContribution& Contribution : Contributions) {
						if (Contribution.MetaHumanCurveName != MetaHumanCurveName) {
							continue;
						}

						const FString PerfectSyncMorphName = FindPerfectSyncMorphTarget(Contribution.PerfectSyncPoseName, MorphNameList);
						const FString ModelMorphName = FindSkeletalMeshMorphTargetName(sk, PerfectSyncMorphName);
						if (ModelMorphName.IsEmpty()) {
							continue;
						}

						auto curveName = GetUniquePoseName(nullptr, "");
						int32 TargetCurveIndex = 0;
						bool bFoundCurve = false;
						for (decltype(auto) Curve : GetCurves()) {
							if (GetCurveName(Curve).ToString().Equals(ModelMorphName, ESearchCase::IgnoreCase)) {
								curveName = GetCurveSmartName(Curve);
								bFoundCurve = true;
								break;
							}
							++TargetCurveIndex;
						}
						if (bFoundCurve == false) {
							curveName = GetUniquePoseName(k, ModelMorphName, true);
							TargetCurveIndex = GetCurves().Num();
						}

#if UE_VERSION_OLDER_THAN(5,2,0)
						ase->RawCurveData.AddCurveData(curveName);
#else
						const FAnimationCurveIdentifier AddedCurveId(curveName, ERawCurveTrackTypes::RCT_Float);
						DataController.AddCurve(AddedCurveId);
#endif

						// Re-resolve the curve index by name instead of trusting the index
						// captured before AddCurve above: the curve array is not guaranteed to
						// simply grow by appending at the previously-observed Num(), which can
						// silently alias this write onto an unrelated existing curve (this is
						// what caused MetaHuman curves such as CTRL_expressions_noseWrinkleR to
						// end up driving an unrelated morph such as EyeWideRight).
						TargetCurveIndex = 0;
						for (decltype(auto) Curve2 : GetCurves()) {
							if (GetCurveName(Curve2).ToString().Equals(ModelMorphName, ESearchCase::IgnoreCase)) {
								break;
							}
							++TargetCurveIndex;
						}
						decltype(auto) Curves = GetCurves();
						auto& Curve = Curves[TargetCurveIndex];
						Curve.SetCurveTypeFlag(AACF_Editable, true);
						if (Curve.Evaluate(PoseIndex - 1) == 0) {
							Curve.UpdateOrAddKey(0, PoseIndex - 1);
						}
						if (Curve.Evaluate(PoseIndex + 1) == 0) {
							Curve.UpdateOrAddKey(0, PoseIndex + 1);
						}
						Curve.UpdateOrAddKey(Contribution.Weight, PoseIndex);

#if UE_VERSION_OLDER_THAN(5,0,0)
#else
						const FAnimationCurveIdentifier CurveId(curveName, ERawCurveTrackTypes::RCT_Float);
						DataController.AddCurve(CurveId);
						DataController.SetCurveKeys(CurveId, Curve.FloatCurve.GetConstRefOfKeys());
#endif

						UE_LOG(LogVRM4ULoader, Log,
							TEXT("[VRM4U MetaHumanPose] curve=%s perfectSyncPose=%s modelMorph=%s poseIndex=%d targetCurveIndex=%d writtenCurveName=%s weight=%f"),
							*MetaHumanCurveName, *Contribution.PerfectSyncPoseName, *ModelMorphName, PoseIndex, TargetCurveIndex,
							*GetCurveName(Curve).ToString(), Contribution.Weight);

						SetPreviewMorphValue(
							PreviewMorphCurves,
							ModelMorphName,
							PoseIndex,
							Contribution.Weight);
					}

					UE_LOG(LogVRM4ULoader, Log, TEXT("[VRM4U MetaHumanPose] pose '%s' -> frame=%d"), *MetaHumanCurveName, PoseIndex);
					MetaHumanPoseFrames.Add({MetaHumanCurveName, PoseIndex});
					SmartNamePoseList.Add(SmartPoseName);
				}
			};

			// vrm blendshape
			{
				for (auto& group : vrmAssetList->VrmMetaObject->BlendShapeGroup) {

					if (group.name == "") continue;
					if (group.BlendShape.Num() == 0) continue;

					if (IsPoseNameAlreadyUsed(group.name)) {
						// Already created as a pose under this exact name earlier in this
						// same import pass - do not add a duplicate-named pose.
						continue;
					}

					auto SmartPoseName = GetUniquePoseName(k, *group.name, true);
					const int32 PoseIndex = SmartNamePoseList.Num();

					// !! vrm blend shape !!
					bool addCurve = false;
					for (auto& shape : group.BlendShape) {
						if (shape.morphTargetName == "") continue;

						auto curveName = GetUniquePoseName(nullptr, "");
						int targetNo = 0;

						bool bSameName = false;
						if (group.name.ToLower() == shape.morphTargetName.ToLower()) {
							// same name
							curveName = SmartPoseName;
							bSameName = true;
						} else {
							bool bFind = false;
							for (decltype(auto) c : GetCurves()) {
								if (GetCurveName(c).ToString().ToLower() == shape.morphTargetName.ToLower()) {
									// found curve name in list
									bFind = true;
									curveName = GetCurveSmartName(c);
									break;
								}
								++targetNo;
							}
							if (bFind == false) {
								// create new curve
								curveName = GetUniquePoseName(k, *shape.morphTargetName, true);
								targetNo = GetCurves().Num();
							}
						}
						{
#if	UE_VERSION_OLDER_THAN(5,0,0)
							// morph search出来ないのでスキップ
#else

							// Poseで登録しようとする名前と 同じMorphがある場合はスキップ
							auto& MorphList = sk->GetMorphTargets();
							auto* ind = MorphList.FindByPredicate([&SmartPoseName](const TObjectPtr<UMorphTarget > morph) {
#if UE_VERSION_OLDER_THAN(5,3,0)
								if (morph->GetName().Compare(SmartPoseName.DisplayName.ToString())) return false;
#else
								if (morph->GetName().Compare(SmartPoseName.ToString())) return false;
#endif
								return true;
								});
							if (ind) {
								bSameName = true;
							}
#endif
						}

						{
							// DisplayName check
#if UE_VERSION_OLDER_THAN(5,3,0)
							FName n = VRMUtil::GetSanitizedName(curveName.DisplayName.ToString());
							if (n == NAME_None) {
								continue;
							}
#else
							FName n = VRMUtil::GetSanitizedName(curveName.ToString());
							if (n == NAME_None) {
								continue;
							}
#endif
						}

						SetPreviewMorphValue(
							PreviewMorphCurves,
							FindSkeletalMeshMorphTargetName(sk, shape.morphTargetName),
							PoseIndex,
							static_cast<float>(shape.weight) / 100.f);
						

#if UE_VERSION_OLDER_THAN(5,2,0)
						ase->RawCurveData.AddCurveData(curveName);
#else
						FAnimationCurveIdentifier id(curveName, ERawCurveTrackTypes::RCT_Float);
						DataController.AddCurve(id);
#endif

						if (bSameName == false) {
							// Re-resolve the curve index by name instead of trusting the index
							// captured before AddCurve above: the curve array is not guaranteed to
							// simply grow by appending at the previously-observed Num(), which can
							// silently alias this write onto an unrelated existing curve.
							targetNo = 0;
							for (decltype(auto) c2 : GetCurves()) {
								if (GetCurveName(c2).ToString().ToLower() == shape.morphTargetName.ToLower()) {
									break;
								}
								++targetNo;
							}
							decltype(auto) c = GetCurves();
							auto& a = c[targetNo];

							a.SetCurveTypeFlag(AACF_Editable, true);

							// 0 for prev and forward frame
							if (a.Evaluate(PoseIndex - 1) == 0) {
								a.UpdateOrAddKey(0, PoseIndex - 1);
							}
							if (a.Evaluate(PoseIndex + 1) == 0) {
								a.UpdateOrAddKey(0, PoseIndex + 1);
							}
							a.UpdateOrAddKey((float)(shape.weight) / 100.f, PoseIndex);

#if UE_VERSION_OLDER_THAN(5,0,0)
#else
							FAnimationCurveIdentifier CurveId(curveName, ERawCurveTrackTypes::RCT_Float);
							//DataController.AddCurve(CurveId);
							DataController.SetCurveKeys(CurveId, a.FloatCurve.GetConstRefOfKeys());
#endif
						}
						addCurve = true;
					}

					if (addCurve) {
						// new pose
						SmartNamePoseList.Add(SmartPoseName);
					}
				}
			}

			AddPerfectSyncPoses();
			AddMetaHumanPoses();

			// Use the nearest VRM expression when a model has no direct Perfect Sync morph.
			{
				for (const FPerfectSyncFallbackMapping& FallbackMapping : GetPerfectSyncFallbackMappings()) {
					const FString& PerfectSyncPoseName = FallbackMapping.PerfectSyncPoseName;
					const FString& VrmPoseName = FallbackMapping.VrmPoseName;

					// Prefer a direct Perfect Sync morph when the model provides one.
					{
						const FString* DirectMorph = MorphNameList.FindByPredicate(
							[&PerfectSyncPoseName](const FString& MorphName) {
								return MorphName.Equals(PerfectSyncPoseName, ESearchCase::IgnoreCase);
							});
						if (DirectMorph) {
							continue;
						}
					}

					const int32 PerfectSyncPoseIndex = FindPoseIndexByName(SmartNamePoseList, PerfectSyncPoseName);
					const int32 VrmPoseIndex = FindPoseIndexByName(SmartNamePoseList, VrmPoseName);
					if (PerfectSyncPoseIndex == INDEX_NONE || VrmPoseIndex == INDEX_NONE) {
						continue;
					}

					decltype(auto) curves = GetCurves();

					bool bHasCurve = false;
					for (auto& c : curves) {
						if (c.Evaluate(PerfectSyncPoseIndex) >= 0.5f) {
							bHasCurve = true;
							break;
						}
					}
					if (bHasCurve == true) {
						// has armorph value
						continue;
					}

					for (auto& c : curves) {
						const float VrmPoseValue = c.Evaluate(VrmPoseIndex);
						if (c.Evaluate(PerfectSyncPoseIndex) == VrmPoseValue) {
							continue;
						}

						// 0 for prev and forward frame
						if (c.Evaluate(PerfectSyncPoseIndex - 1) == 0) {
							c.UpdateOrAddKey(0, PerfectSyncPoseIndex - 1);
						}
						if (c.Evaluate(PerfectSyncPoseIndex + 1) == 0) {
							c.UpdateOrAddKey(0, PerfectSyncPoseIndex + 1);
						}
						c.UpdateOrAddKey(VrmPoseValue, PerfectSyncPoseIndex);

#if UE_VERSION_OLDER_THAN(5,2,0)
#else
						FAnimationCurveIdentifier CurveId(GetCurveSmartName(c), ERawCurveTrackTypes::RCT_Float);
						DataController.SetCurveKeys(CurveId, c.FloatCurve.GetConstRefOfKeys());
#endif
					}

					for (FPreviewMorphCurve& PreviewMorphCurve : PreviewMorphCurves) {
						const float* VrmPoseValue = PreviewMorphCurve.ValuesByPose.Find(VrmPoseIndex);
						if (VrmPoseValue) {
							PreviewMorphCurve.ValuesByPose.Add(PerfectSyncPoseIndex, *VrmPoseValue);
						}
					}
				}
			}


#if UE_VERSION_OLDER_THAN(5,0,0)
			ase->SetRawNumberOfFrame(SmartNamePoseList.Num());
#endif

			ase->RateScale = 24.f;

#if UE_VERSION_OLDER_THAN(5,0,0)
			ase->SequenceLength = float(SmartNamePoseList.Num() - 1);
#elif UE_VERSION_OLDER_THAN(5,2,0)
			{
				DataController.SetPlayLength(float(SmartNamePoseList.Num() - 1));

				FFrameRate f(1, 1);
				DataController.SetFrameRate(f);

				DataController.UpdateCurveNamesFromSkeleton(k, ERawCurveTrackTypes::RCT_Float);
				DataController.NotifyPopulated();
			}
#else
			{
				//DataController.InitializeModel();
				FFrameRate ff(1, 1); 
				//DataController.SetFrameRate(FCommonFrameRates::FPS_30());
				DataController.SetFrameRate(ff);
				DataController.SetNumberOfFrames(SmartNamePoseList.Num() - 1);

				ase->SetPreviewMesh(sk);

#if	UE_VERSION_OLDER_THAN(5,3,0)
				DataController.UpdateCurveNamesFromSkeleton(k, ERawCurveTrackTypes::RCT_Float);
#endif
				DataController.NotifyPopulated();
			}
#endif
		}

		if (SmartNamePoseList.Num() > 0) {
			{
				// Diagnostic dump: list every pose name in order along with its frame
				// index, and flag any duplicate name (a duplicate would make
				// CreatePoseFromAnimation associate more than one MetaHuman/PerfectSync
				// curve with the same pose, which looks like "the wrong morph moves").
				TMap<FString, int32> SeenPoseNames;
				for (int32 DumpIndex = 0; DumpIndex < SmartNamePoseList.Num(); ++DumpIndex) {
#if UE_VERSION_OLDER_THAN(5,3,0)
					const FString DumpPoseName = SmartNamePoseList[DumpIndex].DisplayName.ToString();
#else
					const FString DumpPoseName = SmartNamePoseList[DumpIndex].ToString();
#endif
					UE_LOG(LogVRM4ULoader, Log, TEXT("[VRM4U PoseDump] frame=%d pose=%s"), DumpIndex, *DumpPoseName);

					const int32* PrevIndex = SeenPoseNames.Find(DumpPoseName.ToLower());
					if (PrevIndex) {
						UE_LOG(LogVRM4ULoader, Warning, TEXT("[VRM4U PoseDump] DUPLICATE pose name '%s' at frame=%d (first seen at frame=%d)"), *DumpPoseName, DumpIndex, *PrevIndex);
					} else {
						SeenPoseNames.Add(DumpPoseName.ToLower(), DumpIndex);
					}
				}
			}
			pose->CreatePoseFromAnimation(ase, &SmartNamePoseList);
#if	UE_VERSION_OLDER_THAN(5,3,0)
#else
			pose->UpdatePoseFromAnimation(ase);
#endif
			// for additive
			pose->ConvertSpace(false, 0);
			pose->ConvertSpace(true, 0);

		}
		AddMetaHumanMarkerCurves(ase, k, MetaHumanPoseFrames, SmartNamePoseList.Num());
		UAnimSequence* FacePreviewAnimation = CreateFacePreviewAnimation(vrmAssetList, sk, k, PreviewMorphCurves, SmartNamePoseList.Num());
		AddMetaHumanMarkerCurves(FacePreviewAnimation, k, MetaHumanPoseFrames, SmartNamePoseList.Num());
#if	UE_VERSION_OLDER_THAN(5,0,0)
		ase->PreSave(nullptr);
#else
#endif
		ase->PostEditChange();

	} // AnimSequence



#endif
}// namespace


bool VRMConverter::ConvertPose(UVrmAssetListObject *vrmAssetList) {

	if (VRMConverter::Options::Get().IsDebugOneBone() || VRMConverter::Options::Get().IsSkipRetargeter()) {
		return true;
	}

	if (vrmAssetList->SkeletalMesh == nullptr) {
		return false;
	}

	bool bPlay = false;
	{
		bool b1, b2, b3;
		b1 = b2 = b3 = false;
		UVrmBPFunctionLibrary::VRMGetPlayMode(b1, b2, b3);
		bPlay = b1 || b2;
	}

#if WITH_EDITOR

	// pose asset
	if (bPlay==false){
		USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
		USkeleton* k = VRMGetSkeleton(sk);

		FString name = FString(TEXT("POSE_retarget_")) + vrmAssetList->BaseFileName;
		
		UPoseAsset *pose = nullptr;

		//if (VRMConverter::Options::Get().IsSingleUAssetFile()) {
			pose = VRM4U_NewObject<UPoseAsset>(vrmAssetList->Package, *name, RF_Public | RF_Standalone);

			vrmAssetList->PoseBody = pose;
		//} else {
		//	FString originalPath = vrmAssetList->Package->GetPathName();
		//	const FString PackagePath = FPaths::GetPath(originalPath);

		//	FString NewPackageName = FPaths::Combine(*PackagePath, *name);
		//	UPackage* Pkg = CreatePackage(nullptr, *NewPackageName);

		//	pose = VRM4U_NewObject<UPoseAsset>(Pkg, *name, RF_Public | RF_Standalone);
		//}



		pose->SetSkeleton(k);
		pose->SetPreviewMesh(sk);
		pose->Modify();

		{
			/*
			type 0:
				poseasset +1: T-pose,
			type 1:
				poseasset +1: A-pose,
				retarget +1 : T-pose or A-pose
			type 2:
				poseasset +1: T-pose(footA)
			type 3:
				poseasset +1: A-pose(footT)
			*/
			enum class PoseType {
				TYPE_T,
				TYPE_A,
			};
			PoseType poseType_hand;
			PoseType poseType_foot;
			for (int poseCount = 0; poseCount < 4; ++poseCount) {

				switch (poseCount) {
				case 0:
					poseType_hand = PoseType::TYPE_T;
					poseType_foot = PoseType::TYPE_T;
					break;
				case 1:
					poseType_hand = PoseType::TYPE_A;
					poseType_foot = PoseType::TYPE_A;
					break;
				case 2:
					poseType_hand = PoseType::TYPE_T;
					poseType_foot = PoseType::TYPE_A;
					break;
				case 3:
				default:
					poseType_hand = PoseType::TYPE_A;
					poseType_foot = PoseType::TYPE_T;
					break;
				}

#if	UE_VERSION_OLDER_THAN(5,0,0)
				auto PersonaToolkit = LocalCreatePersonaToolkit(sk);
				UDebugSkelMeshComponent* PreviewComponent = PersonaToolkit->GetPreviewMeshComponent();
				auto* skc = Cast<USkeletalMeshComponent>(PreviewComponent);

#elif	UE_VERSION_OLDER_THAN(5,1,0)
				USkeletalMeshComponent *skc = nullptr;
				//if (GWorld) {
					ASkeletalMeshActor* ska = GWorld->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FTransform::Identity);
					AutoDestroy autoDestroy(ska);
					skc = Cast<USkeletalMeshComponent>(ska->GetRootComponent());
					skc->SetSkeletalMesh(sk);
				//}
#else
				USkeletalMeshComponent* skc = nullptr;
				//if (GWorld) {
					ASkeletalMeshActor* ska = GWorld->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FTransform::Identity);
					AutoDestroy autoDestroy(ska);
					skc = Cast<USkeletalMeshComponent>(ska->GetRootComponent());
					skc->SetSkeletalMeshAsset(sk);
				//}
#endif



				skc->SetComponentSpaceTransformsDoubleBuffering(false);

				{
					VRMRetargetData retargetData;
					retargetData.Setup(vrmAssetList,
						VRMConverter::Options::Get().IsVRMModel(),
						VRMConverter::Options::Get().IsBVHModel(),
						VRMConverter::Options::Get().IsPMXModel());

					// default A-pose
					//retargetTable = retargetData.retargetTable;

					//TArray<VRMRetargetData::RetargetParts> retargetTable;
					if (VRMConverter::Options::Get().IsVRMModel() || VRMConverter::Options::Get().IsBVHModel()) {

						if (poseType_hand == PoseType::TYPE_T) {
							TArray<FString> strTable = {
								TEXT("Thigh_R"),
								TEXT("Thigh_L"),
								TEXT("calf_r"),
								TEXT("calf_l"),
								TEXT("Foot_R"),
								TEXT("Foot_L") 
							};

							// 手の情報を消す
							bool bLoop = true;
							while (bLoop) {
								bLoop = false;
								for (auto r : retargetData.retargetTable) {
									if (strTable.Find(r.BoneUE4) < 0) {
										retargetData.Remove(r.BoneUE4);
										bLoop = true;
										break;
									}
								}
							}
						}
					}
					if (VRMConverter::Options::Get().IsPMXModel()) {
						if (poseType_hand == PoseType::TYPE_T) {
							auto& poseList = k->GetReferenceSkeleton().GetRefBonePose();
							FString* boneName = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("rightLowerArm"));
							float degRot = 0.f;
							if (boneName) {
								int ind = k->GetReferenceSkeleton().FindBoneIndex(**boneName);
								if (ind >= 0) {
									FVector v = poseList[ind].GetLocation();
									v.Z = FMath::Abs(v.Z);
									v.X = FMath::Abs(v.X);
									degRot = FMath::Abs(FMath::Atan2(v.Z, v.X)) * 180.f / PI;
								}
							}
							if (degRot) {
								{
									VRMRetargetData::RetargetParts t;
									t.BoneUE4 = TEXT("UpperArm_R");
									t.rot = FRotator(-degRot, 0, 0);
									retargetData.Remove(t.BoneUE4);
									retargetData.retargetTable.Push(t);
								}
								{
									VRMRetargetData::RetargetParts t;
									t.BoneUE4 = TEXT("UpperArm_L");
									t.rot = FRotator(degRot, 0, 0);
									retargetData.Remove(t.BoneUE4);
									retargetData.retargetTable.Push(t);
								}
							}
						}
						if (poseType_hand == PoseType::TYPE_A) {
							{
								VRMRetargetData::RetargetParts t;
								t.BoneUE4 = TEXT("lowerarm_r");
								t.rot = FRotator(0, -30, 0);
								retargetData.Remove(t.BoneUE4);
								retargetData.retargetTable.Push(t);
							}
							{
								VRMRetargetData::RetargetParts t;
								t.BoneUE4 = TEXT("Hand_R");
								t.rot = FRotator(10, 0, 0);
								retargetData.Remove(t.BoneUE4);
								retargetData.retargetTable.Push(t);
							}
							{
								VRMRetargetData::RetargetParts t;
								t.BoneUE4 = TEXT("lowerarm_l");
								t.rot = FRotator(-0, 30, 0);
								retargetData.Remove(t.BoneUE4);
								retargetData.retargetTable.Push(t);
							}
							{
								VRMRetargetData::RetargetParts t;
								t.BoneUE4 = TEXT("Hand_L");
								t.rot = FRotator(-10, 0, 0);
								retargetData.Remove(t.BoneUE4);
								retargetData.retargetTable.Push(t);
							}
						}
					}
					if (poseType_foot == PoseType::TYPE_T) {
						// 足の情報を消す
						TArray<FString> strTable = {
							TEXT("Thigh_R"),
							TEXT("Thigh_L"),
							TEXT("calf_r"),
							TEXT("calf_l"),
							TEXT("Foot_R"),
							TEXT("Foot_L")
						};

						for (auto s : strTable) {
							retargetData.Remove(s);
						}
					}

					retargetData.UpdateBoneName();

					for (auto& a : retargetData.retargetTable) {
						int32 BoneIndex = VRMGetRefSkeleton(sk).FindBoneIndex(*a.BoneModel);
						if (BoneIndex < 0) continue;

						FTransform dstTrans;
						auto dstIndex = BoneIndex;
						
						const auto BoneTrans = VRMGetRefSkeleton(sk).GetRefBonePose()[dstIndex];

						while (dstIndex >= 0)
						{
							dstIndex = VRMGetRefSkeleton(sk).GetParentIndex(dstIndex);
							if (dstIndex < 0) {
								break;
							}
							dstTrans = VRMGetRefSkeleton(sk).GetRefBonePose()[dstIndex].GetRelativeTransform(dstTrans);
						}

						// p, y, r
						//a.rot = (FRotator(a.rot.Yaw, a.rot.Pitch, a.rot.Roll));

						auto q = (dstTrans.GetRotation().Inverse() * FQuat(a.rot) * dstTrans.GetRotation());
						//auto q = (dstTrans.GetRotation() * FQuat(a.rot) * dstTrans.GetRotation().Inverse());

						//a.rot = (FRotator(a.rot.Yaw, a.rot.Pitch, -a.rot.Roll));
						//DeltaRotation = FQuat(FRotator(rot.Pitch, rot.Roll, rot.Yaw));
						////DeltaRotation = FQuat(FRotator(rot.Roll, rot.Pitch, rot.Yaw));
						////DeltaRotation = FQuat(FRotator(rot.Yaw, rot.Roll, rot.Pitch));
						//DeltaRotation = FQuat(FRotator(rot.Roll, rot.Yaw, rot.Pitch));
						////DeltaRotation = FQuat(FRotator(rot.Pitch, rot.Yaw, rot.Roll));

						q = BoneTrans.GetRotation() * q;
						//q = q * BoneTrans.GetRotation();
						a.rot = q.Rotator();
					}


					TMap<FString, VRMRetargetData::RetargetParts> mapTable;
					for (auto &a : retargetData.retargetTable) {
						bool bFound = false;
						//vrm
						for (auto &t : VRMUtil::table_ue4_vrm) {
							if (t.BoneUE4.Compare(a.BoneUE4) != 0) {
								continue;
							}
							auto *m = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(t.BoneVRM);
							if (m) {
								bFound = true;
								a.BoneVRM = t.BoneVRM;
								a.BoneModel = *m;
								mapTable.Add(a.BoneModel, a);
							}
							break;
						}
						if (bFound) {
							continue;
						}
						//pmx
						for (auto &t : VRMUtil::table_ue4_pmx) {
							if (t.BoneUE4.Compare(a.BoneUE4) != 0) {
								continue;
							}
							FString pmxBone;
							VRMUtil::GetReplacedPMXBone(pmxBone, t.BoneVRM);

							FString target[2] = {
								pmxBone,
								t.BoneVRM,
							};
							bool finish = false;
							{
								auto* m = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(target[0]);
								if (m) {
									bFound = true;
									a.BoneVRM = target[0];
									a.BoneModel = *m;
									mapTable.Add(a.BoneModel, a);
								}
								finish = true;
							}

							if (finish) break;
						}
						//bvh
						{
							auto *mc = vrmAssetList->HumanoidRig;
							if (mc) {
								const auto& m = mc->GetNodeMappingTable();
								auto* value = m.Find(*a.BoneUE4);
								if (value) {
									bFound = true;
									mapTable.Add(value->ToString(), a);
								}
							}
						}
						if (bFound) {
							continue;
						}
					}

					auto &rk = k->GetReferenceSkeleton();
					auto &dstTrans = skc->GetEditableComponentSpaceTransforms();

					// init retarget pose
					for (int i = 0; i < dstTrans.Num(); ++i) {
						auto &t = dstTrans[i];
						t = rk.GetRefBonePose()[i];
					}
					if (poseCount == 1) {
						VRMSetRetargetBasePose(sk, dstTrans);
					}

					// override
					for (int i = 0; i < dstTrans.Num(); ++i) {
						auto &t = dstTrans[i];

						auto *m = mapTable.Find(rk.GetBoneName(i).ToString());
						if (m) {
							t.SetRotation(FQuat(m->rot));
						}
					}

					// current pose retarget. local
					if (VRMConverter::Options::Get().IsAPoseRetarget() == true) {
						if (poseCount == 1) {
							VRMSetRetargetBasePose(sk, dstTrans);
						}
					}

					// for rig asset. world
					for (int i = 0; i < dstTrans.Num(); ++i) {
						int parent = rk.GetParentIndex(i);
						if (parent == INDEX_NONE) continue;

						dstTrans[i] = dstTrans[i] * dstTrans[parent];
					}
					// ik bone hand
					{
						int32 ik_g = VRMGetRefSkeleton(sk).FindBoneIndex(TEXT("ik_hand_gun"));
						int32 ik_r = VRMGetRefSkeleton(sk).FindBoneIndex(TEXT("ik_hand_r"));
						int32 ik_l = VRMGetRefSkeleton(sk).FindBoneIndex(TEXT("ik_hand_l"));

						if (ik_g >= 0 && ik_r >= 0 && ik_l >= 0) {
							const VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(aiData->mVRMMeta);

							auto ar = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("rightHand"));
							auto al = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("leftHand"));
							if (ar && al) {
								int32 kr = VRMGetRefSkeleton(sk).FindBoneIndex(**ar);
								int32 kl = VRMGetRefSkeleton(sk).FindBoneIndex(**al);

								dstTrans[ik_g] = dstTrans[kr];
								dstTrans[ik_r] = dstTrans[kr];
								dstTrans[ik_l] = dstTrans[kl];

#if	UE_VERSION_OLDER_THAN(5,3,0)
								// local
								if (VRMGetRetargetBasePose(sk).Num()) {
									VRMGetRetargetBasePose(sk)[ik_g] = dstTrans[kr];
									VRMGetRetargetBasePose(sk)[ik_r].SetIdentity();
									VRMGetRetargetBasePose(sk)[ik_l] = dstTrans[kl] * dstTrans[kr].Inverse();
								}
#endif
							}
						}
					}
					// ik bone foot
					{
						int32 ik_r = VRMGetRefSkeleton(sk).FindBoneIndex(TEXT("ik_foot_r"));
						int32 ik_l = VRMGetRefSkeleton(sk).FindBoneIndex(TEXT("ik_foot_l"));

						if (ik_r >= 0 && ik_l >= 0) {
							const VRM::VRMMetadata *meta = reinterpret_cast<VRM::VRMMetadata*>(aiData->mVRMMeta);

							auto ar = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("rightFoot"));
							auto al = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(TEXT("leftFoot"));
							if (ar && al) {
								int32 kr = VRMGetRefSkeleton(sk).FindBoneIndex(**ar);
								int32 kl = VRMGetRefSkeleton(sk).FindBoneIndex(**al);

								dstTrans[ik_r] = dstTrans[kr];
								dstTrans[ik_l] = dstTrans[kl];

#if	UE_VERSION_OLDER_THAN(5,3,0)
								// local
								if (VRMGetRetargetBasePose(sk).Num()) {
									VRMGetRetargetBasePose(sk)[ik_r] = dstTrans[kr];
									VRMGetRetargetBasePose(sk)[ik_l] = dstTrans[kl];
								}
#endif
							}
						}
					}

				}
				{
					auto  PoseName = GetUniquePoseName(nullptr, "");
					switch(poseCount) {
					case 0:
						PoseName = GetUniquePoseName(VRMGetSkeleton(sk), TEXT("POSE_T"), true);
						break;
					case 1:
						PoseName = GetUniquePoseName(VRMGetSkeleton(sk), TEXT("POSE_A"), true);
						break;
					case 2:
						PoseName = GetUniquePoseName(VRMGetSkeleton(sk), TEXT("POSE_T(foot_A)"), true);
						break;
					case 3:
					default:
						PoseName = GetUniquePoseName(VRMGetSkeleton(sk), TEXT("POSE_A(foot_T)"), true);
						break;
					}
					//pose->AddOrUpdatePose(PoseName, Cast<USkeletalMeshComponent>(PreviewComponent));

#if	UE_VERSION_OLDER_THAN(5,3,0)
					FSmartName newName;
					//pose->AddOrUpdatePoseWithUniqueName(Cast<USkeletalMeshComponent>(PreviewComponent), &newName);
					pose->AddOrUpdatePoseWithUniqueName(skc, &newName);
					pose->ModifyPoseName(newName.DisplayName, PoseName.DisplayName, nullptr);
#else
					auto newName = pose->AddPoseWithUniqueName(skc);
					pose->ModifyPoseName(newName, PoseName);
#endif
				}
			}
		}
	}

	bool bUseFace = true;
	if (VRMConverter::Options::Get().IsNoMesh()) {
		bUseFace = false;
	}
	if (VRMConverter::Options::Get().IsBVHModel()) {
		bUseFace = false;
	}
	if (bUseFace){
		localFaceMorphConv(vrmAssetList, aiData);
	}

#endif // editor

	return true;

}



