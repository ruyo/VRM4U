// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmConvertRig.h"
#include "VrmConvert.h"
#include "VrmUtil.h"

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
	static void localFaceMorphConv(UVrmAssetListObject* vrmAssetList, const aiScene* aiData) {
		const TArray<FString> arkitMorphName = {
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
			// arkit blendshape
			for (auto& arkitMorph : arkitMorphName) {

				int no = MorphNameList.Find(arkitMorph);

				FString modelMorph;
				if (no >= 0) {
					modelMorph = arkitMorph;
				} else {
					auto ar = arkitMorph.ToLower();
					FString* find_s = MorphNameList.FindByPredicate(
						[&ar, &modelMorph](const FString& str) {
						//
						auto s = str.ToLower();
						if (s == ar) {
							modelMorph = str;
							return true;
						}
						FString tmp;
						tmp = ar.Replace(TEXT("Left"), TEXT("_L")).ToLower();
						if (s == tmp) {
							modelMorph = str;
							return true;
						}
						tmp = ar.Replace(TEXT("Right"), TEXT("_R")).ToLower();
						if (s == tmp) {
							modelMorph = str;
							return true;
						}
						return false;
					}
					);
					if (find_s) {
						no = MorphNameList.Find(*find_s);
					}
				}
				// no morph. add no weight curve
				//if (no < 0) {
				//	continue;
				//}

				auto SmartPoseName = GetUniquePoseName(k, *arkitMorph, true);
				auto curveName = GetUniquePoseName(nullptr, "");

				int targetNo = 0;
				bool bSameName = false;
				if (arkitMorph == modelMorph) {
					// same name. no curve weight
					curveName = SmartPoseName;
					bSameName = true;
				} else if (modelMorph == "") {
					// no morph. norcurve weight
					curveName = SmartPoseName;
					bSameName = true;
				} else {
					bool bFind = false;
					for (decltype(auto) c : GetCurves()) {
						if (GetCurveName(c).ToString().ToLower() == modelMorph.ToLower()) {
							// found curve in list
							bFind = true;
							curveName = GetCurveSmartName(c);
							break;
						}
						++targetNo;
					}
					if (bFind == false) {
						// create new curve
						curveName = GetUniquePoseName(k, *modelMorph, true);
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
					decltype(auto) c = GetCurves();
					auto& a = c[targetNo];

					a.SetCurveTypeFlag(AACF_Editable, true);

					// for default +1
					int targetFrame = GetCurves().Num() + 1;

					// 0 for prev and forward frame
					if (a.Evaluate(targetFrame - 2) == 0) {
						a.UpdateOrAddKey(0, targetFrame - 2);
					}
					if (a.Evaluate(targetFrame + 0) == 0) {
						a.UpdateOrAddKey(0, targetFrame + 0);
					}
					a.UpdateOrAddKey(1, targetFrame - 1);

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


			// vrm blendshape
			{
				// for add and offset
				int targetFrame = GetCurves().Num() + 1 + 1;

				for (auto& group : vrmAssetList->VrmMetaObject->BlendShapeGroup) {

					if (group.name == "") continue;
					if (group.BlendShape.Num() == 0) continue;

					auto SmartPoseName = GetUniquePoseName(k, *group.name, true);

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
						

#if UE_VERSION_OLDER_THAN(5,2,0)
						ase->RawCurveData.AddCurveData(curveName);
#else
						FAnimationCurveIdentifier id(curveName, ERawCurveTrackTypes::RCT_Float);
						DataController.AddCurve(id);
#endif

						if (bSameName == false) {
							decltype(auto) c = GetCurves();
							auto& a = c[targetNo];

							a.SetCurveTypeFlag(AACF_Editable, true);

							// 0 for prev and forward frame
							if (a.Evaluate(targetFrame - 2) == 0) {
								a.UpdateOrAddKey(0, targetFrame - 2);
							}
							if (a.Evaluate(targetFrame + 0) == 0) {
								a.UpdateOrAddKey(0, targetFrame + 0);
							}
							a.UpdateOrAddKey((float)(shape.weight) / 100.f, targetFrame - 1);

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
						targetFrame += 1;
					}
				}
			}

			// support arkit from vrm
			{
				TArray<FString> BlendFrom = {
					TEXT("EyeBlinkLeft"),
					TEXT("EyeBlinkRight"),
					TEXT("JawOpen"),
				};
				TArray<FString> BlendTo = {
					TEXT("Blink_L"),
					TEXT("Blink_R"),
					TEXT("A"),
				};
				for (auto& arkitMorph : arkitMorphName) {
					int tableNo = BlendFrom.Find(arkitMorph);
					if (tableNo < 0) {
						continue;
					}
					const FString vrmMorph = BlendTo[tableNo];

					// arkit MorphName check
					{
						FString* s = MorphNameList.FindByPredicate(
							[&arkitMorph](const FString& str) {
							return (arkitMorph.ToLower() == str.ToLower());
						});
						if (s) {
							// has same arkit morph name
							continue;
						}
					}

					int arMorphFrameNo = -1;
					{
#if UE_VERSION_OLDER_THAN(5,3,0)
						FSmartName* m = SmartNamePoseList.FindByPredicate(
							[&arkitMorph](FSmartName& name) {
							return (arkitMorph.ToLower() == name.DisplayName.ToString().ToLower());
						});
#else
						FName* m = SmartNamePoseList.FindByPredicate(
							[&arkitMorph](FName& name) {
								return (arkitMorph.ToLower() == name.ToString().ToLower());
							});
#endif
						if (m == nullptr) continue;
						arMorphFrameNo = SmartNamePoseList.Find(*m);
					}
					int vrmMorphFrameNo = -1;
					{
#if UE_VERSION_OLDER_THAN(5,3,0)
						FSmartName* m = SmartNamePoseList.FindByPredicate(
							[&vrmMorph](FSmartName& name) {
							return (vrmMorph.ToLower() == name.DisplayName.ToString().ToLower());
						});
#else
						FName* m = SmartNamePoseList.FindByPredicate(
							[&vrmMorph](FName& name) {
								return (vrmMorph.ToLower() == name.ToString().ToLower());
							});
#endif
						if (m == nullptr) continue;
						vrmMorphFrameNo = SmartNamePoseList.Find(*m);
					}

					decltype(auto) curves = GetCurves();
					if (curves.Num() <= arMorphFrameNo || curves.Num() <= vrmMorphFrameNo) {
						// range check
						continue;
					}

					bool bHasCurve = false;
					for (auto& c : curves) {
						if (c.Evaluate(arMorphFrameNo) >= 0.5f) {
							bHasCurve = true;
							break;
						}
					}
					if (bHasCurve == true) {
						// has armorph value
						continue;
					}

					for (auto& c : curves) {
						if (c.Evaluate(arMorphFrameNo) == c.Evaluate(vrmMorphFrameNo)) {
							continue;
						}

						// 0 for prev and forward frame
						if (c.Evaluate(arMorphFrameNo - 1) == 0) {
							c.UpdateOrAddKey(0, arMorphFrameNo - 1);
						}
						if (c.Evaluate(arMorphFrameNo + 1) == 0) {
							c.UpdateOrAddKey(0, arMorphFrameNo + 1);
						}
						c.UpdateOrAddKey(c.Evaluate(vrmMorphFrameNo), arMorphFrameNo);
					}
				}
#if UE_VERSION_OLDER_THAN(5,0,0)
#else
				//FAnimationCurveIdentifier CurveId(curveName, ERawCurveTrackTypes::RCT_Float);
				//DataController.AddCurve(CurveId);
				//DataController.SetCurveKeys(CurveId, a.FloatCurve.GetConstRefOfKeys());
#endif

			}


#if	UE_VERSION_OLDER_THAN(4,22,0)
			ase->NumFrames = SmartNamePoseList.Num();
#elif UE_VERSION_OLDER_THAN(5,0,0)
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
			pose->CreatePoseFromAnimation(ase, &SmartNamePoseList);
#if	UE_VERSION_OLDER_THAN(5,3,0)
#else
			pose->UpdatePoseFromAnimation(ase);
#endif
			// for additive
			pose->ConvertSpace(false, 0);
			pose->ConvertSpace(true, 0);

		}
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

#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
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
							for (int i = 0; i < 2; ++i) {
								auto* m = vrmAssetList->VrmMetaObject->humanoidBoneTable.Find(target[i]);
								if (m) {
									bFound = true;
									a.BoneVRM = target[i];
									a.BoneModel = *m;
									mapTable.Add(a.BoneModel, a);
								}
								finish = true;
								break;
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
#endif //420

	return true;

}



