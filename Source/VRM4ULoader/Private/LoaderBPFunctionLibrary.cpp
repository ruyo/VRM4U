// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "LoaderBPFunctionLibrary.h"

//#include "VRM4U.h"
#include "VrmSkeletalMesh.h"
#include "VrmModelActor.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmLicenseObject.h"
#include "Vrm1LicenseObject.h"

#include "VrmAsyncLoadAction.h"

#include "VrmConvert.h"
#include "VrmUtil.h"
#include "VRM4ULoaderLog.h"

#include "Components/SkeletalMeshComponent.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/Skeleton.h"
#include "Animation/MorphTarget.h"
#include "Animation/NodeMappingContainer.h"
#include "Animation/PoseAsset.h"

//#include ".h"

#include "RenderingThread.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"
#include "Engine/SkeletalMeshSocket.h"
#include "EditorFramework/AssetImportData.h"
#include "TextureResource.h"
#include "Engine/Texture2D.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Engine/Blueprint.h"

#if	UE_VERSION_OLDER_THAN(4,26,0)
#include "AssetRegistryModule.h"
#else
#include "AssetRegistry/AssetRegistryModule.h"
#endif

#include "UObject/Package.h"
#include "Engine/Engine.h"


//#include "Windows/WindowsSystemIncludes.h"

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
#include "EditorFramework/AssetImportData.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#endif

#endif

#include "VrmRigHeader.h"

#if UE_VERSION_OLDER_THAN(5,4,0)
#else
#include "MIsc/FieldAccessor.h"
#endif

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>

#if	UE_VERSION_OLDER_THAN(4,23,0)
#define TRACE_CPUPROFILER_EVENT_SCOPE(a)
#else
#endif


// tem
namespace {
	UPackage *s_vrm_package = nullptr;
	FString baseFileName;
}

namespace {
	class RenderControl {
		bool tmp = false;
	public:
		RenderControl() {
#if	UE_VERSION_OLDER_THAN(5,0,0)
			tmp = GUseThreadedRendering;

			if (tmp) {
				StopRenderingThread();
				GUseThreadedRendering = false;
			}
#else
#endif
		}
		~RenderControl() {
#if	UE_VERSION_OLDER_THAN(5,0,0)
			if (tmp) {
				GUseThreadedRendering = true;
				StartRenderingThread();
			}
#else
#endif
		}
	};

}


static std::string GetExtAndSetModelTypeLocal(std::string e, const uint8* pDataLocal, size_t sizeLocal) {
	std::string e_tmp = e;
	VRMConverter::Options::Get().ClearModelType();

	if (e.compare("vrm") == 0 || e.compare("glb") == 0 || e.compare("gltf") == 0) {

		VRMConverter::Options::Get().SetVRM0Model(true);

		extern bool VRMIsVRM10(const uint8 * pData, size_t size);
		if (VRMIsVRM10(pDataLocal, sizeLocal)) {
			VRMConverter::Options::Get().SetVRM10Model(true);
		}
	}

	if (e.compare("vrma") == 0) {
		VRMConverter::Options::Get().SetVRMAModel(true);
		VRMConverter::Options::Get().SetNoMesh(true);
		VRMConverter::Options::Get().SetVRM10Model(true);
		e_tmp = "vrm";
	}

	if (e.compare("bvh") == 0) {
		VRMConverter::Options::Get().SetBVHModel(true);
	}
	if (e.compare("pmx") == 0) {
		VRMConverter::Options::Get().SetPMXModel(true);
	}
	return e_tmp;
}

static bool RemoveObject(UObject* u) {
	if (u == nullptr) return true;
#if WITH_EDITOR

	u->ClearFlags(EObjectFlags::RF_Standalone);
	u->SetFlags(EObjectFlags::RF_Public | RF_Transient);
	u->ConditionalBeginDestroy();
#endif
	return true;
}

static bool RemoveAssetList(UVrmAssetListObject *&assetList) {
	if (assetList == nullptr) return false;
#if WITH_EDITOR

	for (auto& t : assetList->Textures) {
		RemoveObject(t);
	}
	for (auto& t : assetList->Materials) {
		RemoveObject(t);
	}
	for (auto& t : assetList->OutlineMaterials) {
		RemoveObject(t);
	}
	if (assetList->SkeletalMesh) {
		RemoveObject(VRMGetSkeleton(assetList->SkeletalMesh));
		RemoveObject(VRMGetPhysicsAsset(assetList->SkeletalMesh));
		RemoveObject(assetList->SkeletalMesh);
	}
	RemoveObject(assetList->VrmMetaObject);
	RemoveObject(assetList->VrmLicenseObject);
	RemoveObject(assetList->Vrm1LicenseObject);
	RemoveObject(assetList->HumanoidSkeletalMesh);
	RemoveObject(assetList->HumanoidRig);

	RemoveObject(assetList);
	assetList = nullptr;
#endif
	return true;
}

static bool RenewPkgAndSaveObject(UObject *u, bool bSave) {
#if WITH_EDITOR
	if (u == nullptr) return false;

	//if (VRMConverter::Options::Get().IsSingleUAssetFile()) {
		if (VRMConverter::IsImportMode()) {
			u->PostEditChange();
		}
		if (bSave) {
			s_vrm_package->MarkPackageDirty();
			FAssetRegistryModule::AssetCreated(u);
#if	UE_VERSION_OLDER_THAN(5,0,0)
			bool bSaved = UPackage::SavePackage(s_vrm_package, u, EObjectFlags::RF_Standalone, *(s_vrm_package->GetName()), GError, nullptr, true, true, SAVE_NoError);
#elif UE_VERSION_OLDER_THAN(5,1,0)
			FSavePackageArgs SaveArgs = { nullptr, EObjectFlags::RF_Standalone, SAVE_NoError, true,
					true, true, FDateTime::MinValue(), GError };
			bool bSaved = UPackage::SavePackage(s_vrm_package, u, *(s_vrm_package->GetName()), SaveArgs);
#else
			FSavePackageArgs SaveArgs = { nullptr, nullptr, EObjectFlags::RF_Standalone, SAVE_NoError, true,
					true, true, FDateTime::MinValue(), GError };
			bool bSaved = UPackage::SavePackage(s_vrm_package, u, *(s_vrm_package->GetName()), SaveArgs);
#endif
		}
		/*
	}else{
		FString objPath = u->GetPathName();
		if (objPath.IsEmpty()) return false;

		const FString PackageName = FPackageName::ObjectPathToPackageName(objPath);
		const FString PackagePath = FPaths::GetPath(PackageName);
		const FString AssetName = FPaths::GetBaseFilename(PackageName);

		FString NewPackageName = FPaths::Combine(*PackagePath, *(u->GetFName().ToString()));
		UPackage* Pkg = CreatePackage(nullptr, *NewPackageName);
		u->Rename(nullptr, Pkg);

		if (VRMConverter::IsImportMode()) {
			u->PostEditChange();
		}

		if (bSave) {
			UPackage::SavePackage(Pkg, u, RF_Standalone,
				*FPackageName::LongPackageNameToFilename(NewPackageName, FPackageName::GetAssetPackageExtension()),
				GError, nullptr, false, true, SAVE_NoError);
		}
	}
	*/

#endif
	return true;
}

static UTexture2D* LocalGetTexture(const aiScene* mScenePtr, int texIndex) {

	if (texIndex < 0 || texIndex >= (int)mScenePtr->mNumTextures) {
		return nullptr;
	}

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper;
	// Note: PNG format.  Other formats are supported
	ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

	auto& t = *mScenePtr->mTextures[texIndex];
	int Width = t.mWidth;
	int Height = t.mHeight;
#if	UE_VERSION_OLDER_THAN(4,25,0)
#else
	TArray<uint8> RawData;
#endif
	const TArray<uint8>* pRawData = nullptr;

	if (Height == 0) {
		if (ImageWrapper->SetCompressed(t.pcData, t.mWidth)) {

		}
		Width = ImageWrapper->GetWidth();
		Height = ImageWrapper->GetHeight();

		if (Width == 0 || Height == 0) {
			return nullptr;
		}

#if	UE_VERSION_OLDER_THAN(4,25,0)
		ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, pRawData);
#else
		ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData);
		pRawData = &RawData;
#endif
	}
	FString baseName;

	auto *NewTexture2D = VRMLoaderUtil::CreateTexture(Width, Height, FString(TEXT("T_")) + baseName, GetTransientPackage());
	//UTexture2D* NewTexture2D = _CreateTransient(Width, Height, PF_B8G8R8A8, t.mFilename.C_Str());

	// Fill in the base mip for the texture we created
	uint8* MipData = (uint8*)GetPlatformData(NewTexture2D)->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	if (pRawData) {
		FMemory::Memcpy(MipData, pRawData->GetData(), pRawData->Num());
	}
	else {
		for (int32 y = 0; y < Height; y++)
		{
			const aiTexel* c = &(t.pcData[y * Width]);
			uint8* DestPtr = &MipData[y * Width * sizeof(FColor)];
			for (int32 x = 0; x < Width; x++)
			{
				*DestPtr++ = c->b;
				*DestPtr++ = c->g;
				*DestPtr++ = c->r;
				*DestPtr++ = c->a;
				c++;
			}
		}
	}
	GetPlatformData(NewTexture2D)->Mips[0].BulkData.Unlock();

	// Set options
	NewTexture2D->SRGB = true;// bUseSRGB;
	NewTexture2D->CompressionSettings = TC_Default;
	NewTexture2D->AddressX = TA_Wrap;
	NewTexture2D->AddressY = TA_Wrap;

#if WITH_EDITORONLY_DATA
	NewTexture2D->CompressionNone = false;
	NewTexture2D->DeferCompression = true;

	// nomipmap for tmporary thumbnail
	//if (VRMConverter::Options::Get().IsMipmapGenerateMode()) {
	//	NewTexture2D->MipGenSettings = TMGS_FromTextureGroup;
	//} else {
	NewTexture2D->MipGenSettings = TMGS_NoMipmaps;
	//}
	NewTexture2D->Source.Init(Width, Height, 1, 1, ETextureSourceFormat::TSF_BGRA8, pRawData->GetData());
	//NewTexture2D->Source.Compress();
#endif

// Update the remote texture data
	NewTexture2D->UpdateResource();
#if WITH_EDITOR
	NewTexture2D->PostEditChange();
#endif
	return NewTexture2D;
}

static void UpdateProgress(int prog) {
#if WITH_EDITOR
	GWarn->UpdateProgress( prog, 100 );
#endif
}

////

static void ReTransformHumanoidBone(USkeleton *targetHumanoidSkeleton, const UVrmMetaObject *meta, const USkeleton *displaySkeleton) {

	FReferenceSkeleton &ReferenceSkeleton = const_cast<FReferenceSkeleton&>(targetHumanoidSkeleton->GetReferenceSkeleton());
	auto &allbone = const_cast<TArray<FMeshBoneInfo> &>(targetHumanoidSkeleton->GetReferenceSkeleton().GetRawRefBoneInfo());

	//auto &humanoidTrans = humanoidSkeleton->GetReferenceSkeleton().GetRawRefBonePose();

	FReferenceSkeletonModifier RefSkelModifier(ReferenceSkeleton, targetHumanoidSkeleton);

	for (int ind_target = 0; ind_target < targetHumanoidSkeleton->GetReferenceSkeleton().GetRawBoneNum(); ++ind_target) {
		FTransform t;
		t.SetIdentity();
		RefSkelModifier.UpdateRefPoseTransform(ind_target, t);

		auto boneName = ReferenceSkeleton.GetBoneName(ind_target);
		//auto &info = targetHumanoidSkeleton->GetReferenceSkeleton().GetRefBoneInfo();
		//auto &a = info[ind_target];

		int32 ind_disp = 0;

		if (meta) {
			auto p = meta->humanoidBoneTable.Find(boneName.ToString());
			if (p == nullptr) {
				continue;
			}
			ind_disp = displaySkeleton->GetReferenceSkeleton().FindBoneIndex(**p);
		}else {
			ind_disp = displaySkeleton->GetReferenceSkeleton().FindBoneIndex(boneName);
		}

		if (ind_disp == INDEX_NONE) {
			continue;
		}
		t = displaySkeleton->GetReferenceSkeleton().GetRefBonePose()[ind_disp];

		auto parent = displaySkeleton->GetReferenceSkeleton().GetParentIndex(ind_disp);
		while (parent != INDEX_NONE) {

			auto s = displaySkeleton->GetReferenceSkeleton().GetBoneName(parent);

			if (meta) {
				if (meta->humanoidBoneTable.FindKey(s.ToString()) != nullptr) {
					// parent == humanoidBone
					break;
				}
			}
			//t.SetLocation(t.GetLocation() + displaySkeleton->GetReferenceSkeleton().GetRefBonePose()[parent].GetLocation());;
			parent = displaySkeleton->GetReferenceSkeleton().GetParentIndex(parent);
		}
		RefSkelModifier.UpdateRefPoseTransform(ind_target, t);
	}

	ReferenceSkeleton.RebuildRefSkeleton(targetHumanoidSkeleton, true);

}

bool ULoaderBPFunctionLibrary::VRMReTransformHumanoidBone(USkeletalMeshComponent *targetHumanoidSkeleton, const UVrmMetaObject *meta, const USkeletalMeshComponent *displaySkeleton) {

	if (targetHumanoidSkeleton == nullptr) return false;
	if (VRMGetSkinnedAsset(targetHumanoidSkeleton) == nullptr) return false;

	if (displaySkeleton == nullptr) return false;
	if (VRMGetSkinnedAsset(displaySkeleton) == nullptr) return false;

	// no meta. use default name.
	//if (meta == nullptr) return false;

	ReTransformHumanoidBone(VRMGetSkeleton( VRMGetSkinnedAsset(targetHumanoidSkeleton) ), meta, VRMGetSkeleton( VRMGetSkinnedAsset(displaySkeleton) ));
	auto *sk = VRMGetSkinnedAsset(targetHumanoidSkeleton);
	auto *k = VRMGetSkeleton(sk);

	VRMSetRefSkeleton(sk, k->GetReferenceSkeleton());
	//sk->RefSkeleton.RebuildNameToIndexMap();

	//sk->RefSkeleton.RebuildRefSkeleton(sk->Skeleton, true);
	//sk->Proc();

	//out->RefSkeleton = sk->RefSkeleton;
	
	VRMSetSkeleton(sk, k);
	VRMSetRefSkeleton(sk, k->GetReferenceSkeleton());

	sk->CalculateInvRefMatrices();
	sk->CalculateExtendedBounds();

#if WITH_EDITORONLY_DATA
	sk->ConvertLegacyLODScreenSize();
#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
	sk->UpdateGenerateUpToData();
#endif
#endif

#if WITH_EDITORONLY_DATA
	k->SetPreviewMesh(sk);
#endif
	k->RecreateBoneTree(sk);

	return true;
}



void ULoaderBPFunctionLibrary::SetImportMode(bool bIm, class UPackage *p) {
	VRMConverter::SetImportMode(bIm);
	s_vrm_package = p;
}

namespace {
#if PLATFORM_WINDOWS
	std::string utf_16_to_shift_jis(const std::wstring& str) {
		static_assert(sizeof(wchar_t) == 2, "this function is windows only");
		const int len = ::WideCharToMultiByte(932/*CP_ACP*/, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string re(len * 2, '\0');
		if (!::WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, &re[0], len, nullptr, nullptr)) {
			const auto ec = ::GetLastError();
			switch (ec)
			{
			case ERROR_INSUFFICIENT_BUFFER:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INSUFFICIENT_BUFFER"); break;
			case ERROR_INVALID_FLAGS:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INVALID_FLAGS"); break;
			case ERROR_INVALID_PARAMETER:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: ERROR_INVALID_PARAMETER"); break;
			default:
				//throw std::runtime_error("in function utf_16_to_shift_jis, WideCharToMultiByte fail. cause: unknown(" + std::to_string(ec) + ')'); break;
				break;
			}
		}
		const std::size_t real_len = std::strlen(re.c_str());
		re.resize(real_len);
		re.shrink_to_fit();
		return re;
	}
#endif
}

bool ULoaderBPFunctionLibrary::IsValidVRM4UFile(FString filepath) {

	UE_LOG(LogVRM4ULoader, Log, TEXT("IsValidVRM: OrigFileName=%s"), *filepath);

	const FString ext = FPaths::GetExtension(filepath);
	if (ext.Compare(TEXT("vrm"), ESearchCase::IgnoreCase) && ext.Compare(TEXT("vrma"), ESearchCase::IgnoreCase)) {
		// vrm, vrma以外は素通し
		return true;
	}

	std::string file;
#if PLATFORM_WINDOWS
	file = utf_16_to_shift_jis(*filepath);
#else
	file = TCHAR_TO_UTF8(*filepath);
#endif

	UE_LOG(LogVRM4ULoader, Log, TEXT("IsValidVRM: std::stringFileName=%hs"), file.c_str());

	TArray<uint8> Res;
	if (FFileHelper::LoadFileToArray(Res, *filepath)) {
		UE_LOG(LogVRM4ULoader, Log, TEXT("IsValidVRM: filesize=%d"), Res.Num());

		extern bool VRMIsValid(const uint8_t * pData, size_t size);
			
		return VRMIsValid(Res.GetData(), Res.Num());
	}
	return false;
}

void ULoaderBPFunctionLibrary::GetVRMMeta(FString filepath, UVrmLicenseObject*& a, UVrm1LicenseObject*& b) {

	UE_LOG(LogVRM4ULoader, Log, TEXT("GetVRMMeta:OrigFileName=%s"), *filepath);

	std::string file;
#if PLATFORM_WINDOWS
	file = utf_16_to_shift_jis(*filepath);
#else
	file = TCHAR_TO_UTF8(*filepath);
#endif

	UE_LOG(LogVRM4ULoader, Log, TEXT("GetVRMMeta:std::stringFileName=%hs"), file.c_str());

	Assimp::Importer mImporter;
	mImporter.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
	const aiScene *mScenePtr = nullptr; // delete by Assimp::Importer::~Importer

	VRMConverter vc;
	{
		TArray<uint8> Res;
		if (FFileHelper::LoadFileToArray(Res, *filepath)) {
		}
		UE_LOG(LogVRM4ULoader, Log, TEXT("GetVRMMeta: filesize=%d"), Res.Num());

		const FString ext = FPaths::GetExtension(filepath);
#if PLATFORM_WINDOWS
		std::string e = utf_16_to_shift_jis(*ext);
#else
		std::string e = TCHAR_TO_UTF8(*ext);
#endif

		e = GetExtAndSetModelTypeLocal(e, Res.GetData(), Res.Num());

		mScenePtr = mImporter.ReadFileFromMemory(Res.GetData(), Res.Num(),
			aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes,
			e.c_str());

		UE_LOG(LogVRM4ULoader, Log, TEXT("GetVRMMeta: mScenePtr=%p"), mScenePtr);

		if (mScenePtr == nullptr) {
			return;
		}

		//UE_LOG(LogVRM4ULoader, Log, TEXT("VRM:(%3.3lf secs) ReadFileFromMemory"), FPlatformTime::Seconds() - StartTime);


		{
			// vrm version check

			extern bool VRMIsVRM10(const uint8 * pData, size_t size);
			if (VRMIsVRM10(Res.GetData(), Res.Num())) {
				VRMConverter::Options::Get().SetVRM10Model(true);
				vc.Init(Res.GetData(), Res.Num(), nullptr);
			}
		}
	}

	UTexture2D* NewTexture2D = nullptr;

	if (VRMConverter::Options::Get().IsVRM10Model()) {
		int texIndex = vc.GetThumbnailTextureIndex();
		NewTexture2D = LocalGetTexture(mScenePtr, texIndex);
	}else{
		VRM::VRMMetadata* meta = reinterpret_cast<VRM::VRMMetadata*>(mScenePtr->mVRMMeta);

		if (meta) {
			for (int i = 0; i < meta->license.licensePairNum; ++i) {

				auto& p = meta->license.licensePair[i];

				if (FString(TEXT("texture")) != p.Key.C_Str()) {
					continue;
				}

				unsigned int texIndex = FCString::Atoi(*FString(p.Value.C_Str()));
				NewTexture2D = LocalGetTexture(mScenePtr, texIndex);
				if (NewTexture2D) {
					break;
				}
			}
		}
	}

	UVrmLicenseObject* m = nullptr;
	UVrm1LicenseObject* m1 = nullptr;
	vc.GetVRMMeta(mScenePtr, m, m1);

	if (m) m->thumbnail = NewTexture2D;
	if (m1) m1->thumbnail = NewTexture2D;

	a = m;
	b = m1;
}

bool ULoaderBPFunctionLibrary::VRMSetLoadMaterialType(EVRMImportMaterialType type) {
	VRMConverter::Options::Get().SetMaterialType(type);
	return true;
}

bool ULoaderBPFunctionLibrary::LoadVRMFile(const UVrmAssetListObject *InVrmAsset, UVrmAssetListObject *&OutVrmAsset, const FString filepath, const FImportOptionData &OptionForRuntimeLoad) {
	VRMConverter::Options::Get().SetVrmOption(&OptionForRuntimeLoad);
	OutVrmAsset = nullptr;

	return LoadVRMFileLocal(InVrmAsset, OutVrmAsset, filepath);
}

void ULoaderBPFunctionLibrary::LoadVRMFileAsync(const UObject* WorldContextObject, const class UVrmAssetListObject* InVrmAsset, class UVrmAssetListObject*& OutVrmAsset, const FString filepath, const FImportOptionData& OptionForRuntimeLoad, struct FLatentActionInfo LatentInfo) {
	VRMConverter::Options::Get().SetVrmOption(&OptionForRuntimeLoad);
	OutVrmAsset = nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FVrmAsyncLoadAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == NULL)
		{
			FVrmAsyncLoadActionParam p = { InVrmAsset, OutVrmAsset, OptionForRuntimeLoad, filepath, nullptr, 0 };
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FVrmAsyncLoadAction(LatentInfo, p));
		}
	}
	return;
}


bool ULoaderBPFunctionLibrary::LoadVRMFileLocal(const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString filepath) {
	TArray<uint8> Res;
	if (FFileHelper::LoadFileToArray(Res, *filepath)) {
	}

	return LoadVRMFileFromMemory(InVrmAsset, OutVrmAsset, filepath, Res.GetData(), Res.Num());
}

bool ULoaderBPFunctionLibrary::LoadVRMFileFromMemoryDefaultOption(UVrmAssetListObject*& OutVrmAsset, const FString filepath, const uint8* pData, size_t dataSize) {
#if	UE_VERSION_OLDER_THAN(5,0,0)
	TAssetPtr<UClass> c;
#else
	TSoftObjectPtr<UClass> c;
#endif
	if (c == nullptr) {
		FSoftObjectPath r(TEXT("/VRM4U/VrmAssetListObjectBP.VrmAssetListObjectBP"));
		UObject* u = r.TryLoad();
		if (u) {
			c = (UClass*)(Cast<UBlueprint>(u)->GeneratedClass);
		}
	}

	if (c == nullptr) {
		c = UVrmAssetListObject::StaticClass();
	}
#if	UE_VERSION_OLDER_THAN(5,0,0)
	TAssetPtr<UVrmAssetListObject> m = NewObject<UVrmAssetListObject>((UObject*)GetTransientPackage(), c.Get());
#else
	TSoftObjectPtr<UVrmAssetListObject> m = NewObject<UVrmAssetListObject>((UObject*)GetTransientPackage(), c.Get());
#endif

	return LoadVRMFileFromMemory(m.Get(), OutVrmAsset, filepath, pData, dataSize);
}

bool ULoaderBPFunctionLibrary::LoadVRMFileFromMemory(const UVrmAssetListObject *InVrmAsset, UVrmAssetListObject *&OutVrmAsset, const FString filepath, const uint8 *pFileDataData, size_t dataSize) {
	TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("LoadVRMFileFromMemory"))

	OutVrmAsset = nullptr;
	RenderControl _dummy_control;

	if (InVrmAsset == nullptr) {
		return false;
	}

	Assimp::Importer mImporter;
	mImporter.SetPropertyBool(AI_CONFIG_IMPORT_REMOVE_EMPTY_BONES, false);
	const aiScene* mScenePtr = nullptr; // delete by Assimp::Importer::~Importer

	if (filepath.IsEmpty())
	{
	}

	double StartTime = FPlatformTime::Seconds();
	auto LogAndUpdate = [&](FString logname) {
		UE_LOG(LogVRM4ULoader, Log, TEXT("VRM:(%02.2lf secs) %s"), FPlatformTime::Seconds() - StartTime, *logname);
		StartTime = FPlatformTime::Seconds();
	};

	VRMConverter::Options::Get().SetVRM0Model(true);
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("AssImpLoader"))

		const FString ext = FPaths::GetExtension(filepath).ToLower();
#if PLATFORM_WINDOWS
		std::string e = utf_16_to_shift_jis(*ext);
		std::string e_imp = utf_16_to_shift_jis(*ext);
#else
		std::string e = TCHAR_TO_UTF8(*ext);
		std::string e_imp = TCHAR_TO_UTF8(*ext);
#endif

		e_imp = GetExtAndSetModelTypeLocal(e, pFileDataData, dataSize);

		mScenePtr = mImporter.ReadFileFromMemory(pFileDataData, dataSize,
			aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_PopulateArmatureData,
			e_imp.c_str());

		if (mScenePtr == nullptr) {
			std::string file;
#if PLATFORM_WINDOWS
			file = utf_16_to_shift_jis(*filepath);
#else
			file = TCHAR_TO_UTF8(*filepath);
#endif
			mScenePtr = mImporter.ReadFile(file, aiProcess_Triangulate | aiProcess_MakeLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_OptimizeMeshes | aiProcess_PopulateArmatureData);
		}

		UE_LOG(LogVRM4ULoader, Log, TEXT("VRM:(%3.3lf secs) ReadFileFromMemory"), FPlatformTime::Seconds() - StartTime);
		StartTime = FPlatformTime::Seconds();
	}

	UpdateProgress(20);
	if (mScenePtr == nullptr)
	{
		UE_LOG(LogVRM4ULoader, Warning, TEXT("VRM4U: read failure.\n"));
		return false;
	}

	{
		FString fullpath = FPaths::GameUserDeveloperDir() + TEXT("VRM/");
		FString basepath = FPackageName::FilenameToLongPackageName(fullpath);
		//FPackageName::RegisterMountPoint("/VRMImportData/", fullpath);

		baseFileName = FPaths::GetBaseFilename(filepath);


		if (s_vrm_package == nullptr) {
			s_vrm_package = GetTransientPackage();
		}
	}
	UVrmAssetListObject *out = OutVrmAsset;
	if (OutVrmAsset == nullptr) {
		if (s_vrm_package == GetTransientPackage()) {
			out = Cast<UVrmAssetListObject>(StaticDuplicateObject(InVrmAsset, s_vrm_package, NAME_None));
		} else {
			if (InVrmAsset->ReimportBase) {
				out = InVrmAsset->ReimportBase;
			}
			else {
				out = VRM4U_NewObject<UVrmAssetListObject>(s_vrm_package, *(FString(TEXT("VA_")) + VRMConverter::NormalizeFileName(baseFileName) + FString(TEXT("_VrmAssetList"))), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			}
			InVrmAsset->CopyMember(out);
		}
		OutVrmAsset = out;
	}
	if (out == nullptr) {
		UE_LOG(LogVRM4ULoader, Warning, TEXT("VRM4U: no UVrmAssetListObject.\n"));
		return false;
	}

#if WITH_EDITORONLY_DATA
	{
		out->AssetImportData = NewObject<UAssetImportData>(out, TEXT("AssetImportData"));
		out->AssetImportData->Update(filepath);
	}
#endif

	out->FileFullPathName = filepath;
	out->OrigFileName = baseFileName;
	out->BaseFileName = VRMConverter::NormalizeFileName(baseFileName);
	out->Package = s_vrm_package;

	{
		bool ret = true;
		VRMConverter vc;
		vc.Init(pFileDataData, dataSize, mScenePtr);
		vc.ConvertVrmFirst(out, pFileDataData, dataSize);

		LogAndUpdate(TEXT("Begin convert"));
		ret &= vc.NormalizeBoneName(mScenePtr);
		LogAndUpdate(TEXT("NormalizeBoneName"));
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("VRM Texture and Material"))
			ret &= vc.ConvertTextureAndMaterial(out);
			LogAndUpdate(TEXT("ConvertTextureAndMaterial"));
		}
		UpdateProgress(40);
		{
			bool r = vc.ConvertVrmMeta(out, mScenePtr, pFileDataData, dataSize);	// use texture.
			if (VRMConverter::Options::Get().IsVRMModel() == true) {
				ret &= r;
			}
		}
		LogAndUpdate(TEXT("ConvertVrmMeta"));
		UpdateProgress(60);

		{
			TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("VRM Skeleton"))
			ret &= vc.ConvertModel(out);
			LogAndUpdate(TEXT("ConvertModel"));
		}

		//meta rename
		vc.ConvertVrmMetaPost(out, mScenePtr, pFileDataData, dataSize);

		ret &= vc.ConvertRig(out);
		LogAndUpdate(TEXT("ConvertRig"));
		ret &= vc.ConvertIKRig(out);
		LogAndUpdate(TEXT("ConvertIKRig"));
		if (out->bSkipMorphTarget == false) {
			ret &= vc.ConvertMorphTarget(out);
			LogAndUpdate(TEXT("ConvertMorphTarget"));
		}
		ret &= vc.ConvertPose(out);
		LogAndUpdate(TEXT("ConvertPose"));
		ret &= vc.ConvertHumanoid(out);
		LogAndUpdate(TEXT("ConvertHumanoid"));
		UpdateProgress(80);

		OutVrmAsset->MeshReturnedData = nullptr;
		if (ret == false) {
			RemoveAssetList(out);
			return false;
		}
		{
			// 後処理。UE5.7ではCreateMeshDescription を呼ぶため必要
			if (out->SkeletalMesh) {
				out->SkeletalMesh->PostLoad();
			}
		}
	}
	out->VrmMetaObject->SkeletalMesh = out->SkeletalMesh;

	VRMSetPhysicsAsset(out->VrmMetaObject->SkeletalMesh, nullptr);


	{
		TRACE_CPUPROFILER_EVENT_SCOPE(TEXT("VRM Save"))
		
		LogAndUpdate(TEXT("BeginSave"));
		bool b = out->bAssetSave;
		RenewPkgAndSaveObject(out, b);
		for (auto &t : out->Textures) {
			RenewPkgAndSaveObject(t, b);
		}
		for (auto &t : out->Materials) {
			RenewPkgAndSaveObject(t, b);
		}
		for (auto &t : out->OutlineMaterials) {
			RenewPkgAndSaveObject(t, b);
		}
		RenewPkgAndSaveObject(out->SkeletalMesh, b);
		RenewPkgAndSaveObject(VRMGetSkeleton(out->SkeletalMesh), b);
		RenewPkgAndSaveObject(VRMGetPhysicsAsset(out->SkeletalMesh), b);
		RenewPkgAndSaveObject(out->VrmMetaObject, b);
		RenewPkgAndSaveObject(out->VrmHumanoidMetaObject, b);
		RenewPkgAndSaveObject(out->VrmMannequinMetaObject, b);
		RenewPkgAndSaveObject(out->VrmLicenseObject, b);
		RenewPkgAndSaveObject(out->Vrm1LicenseObject, b);
		RenewPkgAndSaveObject(out->HumanoidSkeletalMesh, b);
		RenewPkgAndSaveObject(out->HumanoidRig, b);

		LogAndUpdate(TEXT("Save"));
	}

	if (VRMConverter::IsImportMode()){
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else

		// refresh content browser
		FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		{
			TArray<UObject*> a = { OutVrmAsset };
			//ContentBrowserModule.Get().SyncBrowserToAssets(a);
		}
		{
			auto path = FPaths::GetPath(OutVrmAsset->GetPathName());
			const TArray<FString> b = { path };
			ContentBrowserModule.Get().SyncBrowserToFolders(b);
			ContentBrowserModule.Get().SetSelectedPaths(b, true);
		}

#endif
#endif

	}
	UpdateProgress(100);
	return true;
}


bool ULoaderBPFunctionLibrary::CopyPhysicsAsset(USkeletalMesh *dstMesh, const USkeletalMesh *srcMesh, bool bResetCollisionTransform){
	//GetTransientPackage
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,4,0)
	if (srcMesh == nullptr || dstMesh == nullptr) return false;

	auto  *srcPA = VRMGetPhysicsAsset(srcMesh);
	UPackage *pk = dstMesh->GetOutermost();

	//FString name = srcPA->GetFName().ToString() + TEXT("_copy");
	FString name = dstMesh->GetFName().ToString() + TEXT("_copy");
	name.RemoveFromStart(TEXT("SK_"));
	name = TEXT("PHYS_") + name;

	UPhysicsAsset *dstPA = VRM4U_NewObject<UPhysicsAsset>(pk, *name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	dstPA->Modify();
	dstPA->SetPreviewMesh(dstMesh);

	TArray<FName> addBoneNameList;

	for (auto &a : srcPA->SkeletalBodySetups) {
		auto rigName = VRMGetSkeleton(srcMesh)->GetRigNodeNameFromBoneName(a->BoneName);
		if (rigName.IsNone()) {
			rigName = a->BoneName;
		}

		auto dstBoneName = VRMGetSkeleton(dstMesh)->GetRigBoneMapping(rigName);
		if (dstBoneName.IsNone()) {
			dstBoneName = rigName;
		}

		if (rigName.IsNone()==false && dstBoneName.IsNone()==false){
			if (addBoneNameList.Find(dstBoneName) < 0) {
				USkeletalBodySetup *bs = Cast<USkeletalBodySetup>(StaticDuplicateObject(a, dstPA, NAME_None));
				bs->BoneName = dstBoneName;
				addBoneNameList.Add(dstBoneName);

				auto srcIndex = VRMGetRefSkeleton(srcMesh).FindBoneIndex(a->BoneName);
				auto dstIndex = VRMGetRefSkeleton(dstMesh).FindBoneIndex(dstBoneName);
				FTransform srcTrans, dstTrans;
				while (srcIndex >= 0)
				{
					srcIndex = VRMGetRefSkeleton(srcMesh).GetParentIndex(srcIndex);
					if (srcIndex < 0) {
						break;
					}
					srcTrans = VRMGetRefSkeleton(srcMesh).GetRefBonePose()[srcIndex].GetRelativeTransform(srcTrans);
				}
				while (dstIndex >= 0)
				{
					dstIndex = VRMGetRefSkeleton(dstMesh).GetParentIndex(dstIndex);
					if (dstIndex < 0) {
						break;
					}
					dstTrans = VRMGetRefSkeleton(dstMesh).GetRefBonePose()[dstIndex].GetRelativeTransform(dstTrans);
				}

				if (bResetCollisionTransform) {
						for (int i = 0; i < bs->AggGeom.SphylElems.Num(); ++i) {
						bs->AggGeom.SphylElems[i].Center.Set(0, 0, 0); 
						bs->AggGeom.SphylElems[i].Rotation = FRotator::ZeroRotator; 

						//bs->AggGeom.SphylElems[i].Center.X = 0;// -v.Z;
						//bs->AggGeom.SphylElems[i].Center.Y = 0;// v.Y;
						//bs->AggGeom.SphylElems[i].Center.Z = 0;// v.X;
					}
					for (auto &b : bs->AggGeom.BoxElems) {
						b.Center.Set(0, 0, 0);
						b.Rotation = FRotator::ZeroRotator;
					}
				}
				dstPA->SkeletalBodySetups.Add(bs);
			}
		}
	}
	for (auto &a : srcPA->ConstraintSetup) {

		FName n[3];
		{
			const FName nn[] = {
				a->DefaultInstance.ConstraintBone1,
				a->DefaultInstance.ConstraintBone2,
				a->DefaultInstance.JointName,
			};
			for (int i = 0; i < 3; ++i) {
				n[i] = VRMGetSkeleton(srcMesh)->GetRigNodeNameFromBoneName(nn[i]);
				if (n[i].IsNone()) {
					n[i] = nn[i];
				}
			}
		}
		const auto rigName1 = n[0];
		const auto rigName2 = n[1];
		const auto rigNameJ = n[2];

		//const auto rigName1 = srcMesh->Skeleton->GetRigNodeNameFromBoneName(a->DefaultInstance.ConstraintBone1);
		//const auto rigName2 = srcMesh->Skeleton->GetRigNodeNameFromBoneName(a->DefaultInstance.ConstraintBone2);
		//const auto rigNameJ = srcMesh->Skeleton->GetRigNodeNameFromBoneName(a->DefaultInstance.JointName);

		UPhysicsConstraintTemplate *ct = Cast<UPhysicsConstraintTemplate>(StaticDuplicateObject(a, dstPA, NAME_None));
		ct->DefaultInstance.ConstraintBone1 = VRMGetSkeleton(dstMesh)->GetRigBoneMapping(rigName1);
		ct->DefaultInstance.ConstraintBone2 = VRMGetSkeleton(dstMesh)->GetRigBoneMapping(rigName2);
		ct->DefaultInstance.JointName = VRMGetSkeleton(dstMesh)->GetRigBoneMapping(rigNameJ);

		{
			auto ind1 = VRMGetRefSkeleton(dstMesh).FindBoneIndex(ct->DefaultInstance.ConstraintBone1);
			auto ind2 = VRMGetRefSkeleton(dstMesh).FindBoneIndex(ct->DefaultInstance.ConstraintBone2);

			if (ind1 >= 0 && ind2 >= 0) {
				auto t = VRMGetRefSkeleton(dstMesh).GetRefBonePose()[ind1];
				while (1) {
					ind1 = VRMGetRefSkeleton(dstMesh).GetParentIndex(ind1);
					if (ind1 == ind2) {
						break;
					}
					if (ind1 < 0) {
						break;
					}
					auto v = VRMGetRefSkeleton(dstMesh).GetRefBonePose()[ind1].GetLocation();
					t.SetLocation(t.GetLocation() + v);
				}
				//auto indParent = dstMesh->RefSkeleton.GetParentIndex(ind1);

				ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame1, FTransform::Identity);
				ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame2, t);
			}
		}
		{
			/*
			auto s1 = a->DefaultInstance->ProfileInstance.ConeLimit.Swing1Motion;
			auto s2 = a->DefaultInstance->ProfileInstance.ConeLimit.Swing1LimitDegrees;
			ct->DefaultInstance.SetAngularTwistLimit(s1, s2);

			s1 = a->DefaultInstance->ProfileInstance.ConeLimit.Swing2Motion;
			s2 = a->DefaultInstance->ProfileInstance.ConeLimit.Swing2LimitDegrees;

			//ct->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 10);
			//ct->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 10);
			*/
		}

		dstPA->ConstraintSetup.Add(ct);
	}

	dstPA->UpdateBodySetupIndexMap();

	dstPA->RefreshPhysicsAssetChange();
	dstPA->UpdateBoundsBodiesArray();
#endif // 5.4
#endif // editor

	return true;
}


bool ULoaderBPFunctionLibrary::CopyVirtualBone(USkeletalMesh *dstMesh, const USkeletalMesh *srcMesh) {
	if (dstMesh == nullptr || srcMesh == nullptr) {
		return false;
	}
#if	UE_VERSION_OLDER_THAN(5,4,0)

	// virtual bone
	{
		const TArray<FVirtualBone>& vTable = VRMGetSkeleton(srcMesh)->GetVirtualBones();

		for (auto &t : vTable) {

			int32 id[] = {
				VRMGetRefSkeleton(dstMesh).FindBoneIndex(t.SourceBoneName),
				VRMGetRefSkeleton(dstMesh).FindBoneIndex(t.TargetBoneName),
			};
			FName n[2] = {
				t.SourceBoneName,
				t.TargetBoneName,
			};

#if WITH_EDITOR
			{
				auto t1 = VRMGetSkeleton(srcMesh)->GetRigNodeNameFromBoneName(t.SourceBoneName);
				auto t2 = VRMGetSkeleton(srcMesh)->GetRigNodeNameFromBoneName(t.TargetBoneName);

				if (t1.IsNone()==false && t2.IsNone()==false) {
					auto r1 = VRMGetSkeleton(dstMesh)->GetRigBoneMapping(t1);
					auto r2 = VRMGetSkeleton(dstMesh)->GetRigBoneMapping(t2);

					if (r1.IsNone()==false && r2.IsNone()==false) {
						id[0] = VRMGetRefSkeleton(dstMesh).FindBoneIndex(r1);
						id[1] = VRMGetRefSkeleton(dstMesh).FindBoneIndex(r2);

						n[0] = r1;
						n[1] = r2;
					}
				}
			}
#endif

			if (id[0] < 0) {
				n[0] = VRMGetRefSkeleton(dstMesh).GetBoneName(0);
			}
			if (id[1] < 0) {
				if (id[0] < 0) {
					continue;
					//n[1] = VRMGetRefSkeleton(dstMesh).GetBoneName(1);
				}
			}

			FName newName;
			if (VRMGetSkeleton(dstMesh)->AddNewVirtualBone(n[0], n[1], newName)) {
				VRMGetSkeleton(dstMesh)->RenameVirtualBone(newName, t.VirtualBoneName);
			}
		}
	}

	// socket
	{
		for (auto &t : VRMGetSkeleton(srcMesh)->Sockets) {
			int32 id = VRMGetRefSkeleton(dstMesh).FindBoneIndex(t->BoneName);

			FName n = t->BoneName;

			FRotator rot = t->RelativeRotation;
			FVector vec = t->RelativeLocation;

#if WITH_EDITOR
			{
				auto t1 = VRMGetSkeleton(srcMesh)->GetRigNodeNameFromBoneName(t->BoneName);

				if (t1.IsNone() == false) {
					auto r1 = VRMGetSkeleton(dstMesh)->GetRigBoneMapping(t1);

					if (r1.IsNone() == false) {
						id = VRMGetRefSkeleton(dstMesh).FindBoneIndex(r1);
						n = r1;
					}
				}
			}
#endif

			if (id < 0) {
				n = VRMGetRefSkeleton(dstMesh).GetBoneName(0);
			}

			bool bNewSocket = false;
			USkeletalMeshSocket *s = nullptr;
			s = VRMGetSkeleton(dstMesh)->FindSocket(t->SocketName);
			if (s == nullptr) {
				s = NewObject<USkeletalMeshSocket>(dstMesh);
				bNewSocket = true;
			}

			{
				int32 bone1 = VRMGetRefSkeleton(srcMesh).FindBoneIndex(t->BoneName);
				int32 bone2 = VRMGetRefSkeleton(dstMesh).FindBoneIndex(n);

				auto getBoneTransform = [](auto &mesh, int32 bone) {
					auto &f = VRMGetRefSkeleton(mesh).GetRefBonePose();
					if (bone < f.Num()) return f[bone];
					return FTransform();
				};

				FTransform f1;
				while (bone1 >= 0) {
					f1 = f1 * getBoneTransform(srcMesh, bone1);
					bone1 = VRMGetRefSkeleton(srcMesh).GetParentIndex(bone1);
				}

				FTransform f2;
				while (bone2 >= 0) {
					f2 = f2 * getBoneTransform(dstMesh, bone2);
					bone2 = VRMGetRefSkeleton(dstMesh).GetParentIndex(bone2);
				}

				FTransform org;
				org.SetLocation(vec);

				f1.SetLocation(FVector::ZeroVector);
				f2.SetLocation(FVector::ZeroVector);

				FTransform tmp = f2 * f1.Inverse() * org * f1 * f2.Inverse();
				vec = tmp.GetLocation();
			}

			s->SocketName = t->SocketName;
			s->BoneName = n;
			s->RelativeLocation = vec;
			s->RelativeRotation = rot;
			s->RelativeScale = t->RelativeScale;
			s->bForceAlwaysAnimated = t->bForceAlwaysAnimated;

			if (bNewSocket) {
				VRMGetSkeleton(dstMesh)->Sockets.Add(s);
			}
		}
	}
	VRMGetSkeleton(dstMesh)->MarkPackageDirty();
#else
	// todo
#endif
	return true;
}


bool ULoaderBPFunctionLibrary::CreateTailBone(USkeletalMesh *skeletalMesh, const TArray<FString> &boneName) {

	if (skeletalMesh == nullptr) {
		return false;
	}

	TArray<int32> tail;
	for (auto &b : boneName) {
		int first = VRMGetRefSkeleton(skeletalMesh).FindBoneIndex(*b);
		if (first < 0) {
			continue;
		}
		tail.AddUnique(first);
	}
	if (tail.Num() == 0) {
		return false;
	}

	USkeletalMesh *sk_tmp = skeletalMesh;// NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
	USkeleton *k_tmp = VRMGetSkeleton(skeletalMesh);//NewObject<UVrmSkeleton>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);

	/*
	k_tmp->RecreateBoneTree(skeletalMesh);
	sk_tmp->Skeleton = k_tmp;
	sk_tmp->RefSkeleton = k_tmp->GetReferenceSkeleton();
	sk_tmp->Skeleton->MergeAllBonesToBoneTree(sk_tmp);
	*/

	for (int i = 0; i < tail.Num(); ++i) {
		TArray<int32> children;
		VRMUtil::GetDirectChildBones(VRMGetRefSkeleton(sk_tmp), tail[i], children);
		if (children.Num()) {
			tail.RemoveAt(i);
			tail.Append(children);
			--i;
		}
	}

	bool bAdd = false;

	FReferenceSkeletonModifier RefSkelModifier(VRMGetRefSkeleton(sk_tmp), VRMGetSkeleton(sk_tmp));
	for (int i = 0; i < tail.Num(); ++i) {
		FMeshBoneInfo b;
		FTransform t;

		b.ParentIndex = tail[i];
		b.Name = VRMGetRefSkeleton(sk_tmp).GetBoneName(tail[i]);

		FString text = TEXT("_vrm4u_dummy_tail");

		if (b.Name.ToString().Contains(text)) {
			continue;
		}
		b.Name = *(b.Name.ToString() + text);
		t = VRMGetRefSkeleton(sk_tmp).GetRefBonePose()[tail[i]];

		RefSkelModifier.Add(b, t);
		bAdd = true;
	}

	if (bAdd) {
		VRMGetRefSkeleton(sk_tmp).RebuildRefSkeleton(VRMGetSkeleton(sk_tmp), true);

		VRMGetSkeleton(skeletalMesh)->ClearCacheData();

		VRMGetSkeleton(skeletalMesh)->MergeAllBonesToBoneTree(sk_tmp);

		//skeletalMesh->RefSkeleton.RebuildRefSkeleton(skeletalMesh->Skeleton, true);

		{
			FSkeletalMeshLODRenderData &rd = skeletalMesh->GetResourceForRendering()->LODRenderData[0];

			{
				int boneNum = VRMGetSkeleton(skeletalMesh)->GetReferenceSkeleton().GetRawBoneNum();
				rd.RequiredBones.SetNum(boneNum);
				rd.ActiveBoneIndices.SetNum(boneNum);

#if WITH_EDITOR
				FSkeletalMeshLODModel *p = &(skeletalMesh->GetImportedModel()->LODModels[0]);
				p->ActiveBoneIndices.SetNum(boneNum);
				p->RequiredBones.SetNum(boneNum);
#endif

				for (int i = 0; i < boneNum; ++i) {
					rd.RequiredBones[i] = i;
					rd.ActiveBoneIndices[i] = i;

#if WITH_EDITOR
					p->ActiveBoneIndices[i] = i;
					p->RequiredBones[i] = i;
#endif
				}
			}
		}

		skeletalMesh->Modify();
		VRMGetSkeleton(skeletalMesh)->Modify();

#if WITH_EDITOR
		skeletalMesh->PostEditChange();
		VRMGetSkeleton(skeletalMesh)->PostEditChange();
#endif
	}

	return true;
}

#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else

static void LocalEpicSkeletonSetup(UIKRigController *rigcon) {
	if (rigcon == nullptr) return;

	rigcon->SetRetargetRoot(TEXT("Pelvis"));
	while (rigcon->GetRetargetChains().Num()) {
		rigcon->RemoveRetargetChain(rigcon->GetRetargetChains()[0].ChainName);
	}
	VRMAddRetargetChain(rigcon, TEXT("root"), TEXT("root"), TEXT("root"));


	int sol_index = 0;
#if	UE_VERSION_OLDER_THAN(5,2,0)
	auto* sol = rigcon->GetSolver(sol_index);
#else
	auto* sol = rigcon->GetSolverAtIndex(sol_index);
#endif

	if (sol == nullptr) {
#if	UE_VERSION_OLDER_THAN(5,2,0)
		sol_index = rigcon->AddSolver(UIKRigPBIKSolver::StaticClass());
		sol = rigcon->GetSolver(sol_index);
#elif UE_VERSION_OLDER_THAN(5,6,0)
		sol_index = rigcon->AddSolver(UIKRigFBIKSolver::StaticClass());
		sol = rigcon->GetSolverAtIndex(sol_index);
#else
		sol_index = rigcon->AddSolver(FIKRigFullBodyIKSolver::StaticStruct());
		sol = rigcon->GetSolverAtIndex(sol_index);
#endif
	}
	if (sol == nullptr) return;
#if	UE_VERSION_OLDER_THAN(5,6,0)
	sol->SetRootBone(TEXT("root"));
#else
	sol->SetStartBone(TEXT("root"));
#endif

	{
		TArray<FString> a = {
			TEXT("Hand_L"),
			TEXT("Hand_R"),
			TEXT("ball_l"),
			TEXT("ball_r"),
		};
		for (int i = 0; i < a.Num(); ++i) {
#if	UE_VERSION_OLDER_THAN(5,2,0)
			auto* goal = rigcon->AddNewGoal(*(a[i] + TEXT("_Goal")), *a[i]);
			if (goal) {
				rigcon->ConnectGoalToSolver(*goal, sol_index);
			}
#else
			auto goal = rigcon->AddNewGoal(*(a[i] + TEXT("_Goal")), *a[i]);
			if (goal != NAME_None) {
				rigcon->ConnectGoalToSolver(goal, sol_index);
			}
#endif
		}
	}
}
#endif
#endif

//void ULoaderBPFunctionLibrary::VRMGenerateEpicSkeletonToHumanoidIKRig(USkeletalMesh *skeletalMesh, UObject*& rig_o, UObject*& ikr_o){
void ULoaderBPFunctionLibrary::VRMGenerateEpicSkeletonToHumanoidIKRig(USkeletalMesh * srcSkeletalMesh, UObject * &outRigIK, UObject * &outIKRetargeter, UObject * targetRigIK){

#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
	USkeletalMesh* sk = srcSkeletalMesh;
	if (sk == nullptr) {
		return;
	}

	auto LocalGetController = [](UIKRigDefinition * rig) {
#if	UE_VERSION_OLDER_THAN(5,2,0)
		return UIKRigController::GetIKRigController(rig);
#else
		return UIKRigController::GetController(rig);
#endif
	};

	{
		const FString PkgPath = sk->GetPathName();// GetPackage().pathn
		const FString SavePackagePath = FPaths::GetPath(PkgPath);

		UIKRigDefinition* rig = nullptr;
		{
			FString name = FString(TEXT("IK_")) + sk->GetName() + TEXT("_VrmHumanoid");
			UPackage* pkg = CreatePackage(*(SavePackagePath +"/"+ name));

			{
				auto* a = FindObject<UIKRigDefinition>(NULL, *(SavePackagePath + "/" + name + TEXT(".") + name));
				if (a != nullptr) {
					rig = a;
				}
			}
			if (rig == nullptr) {
				rig = VRM4U_NewObject<UIKRigDefinition>(pkg, *name, RF_Public | RF_Standalone);
			}

			UIKRigController* rigcon = LocalGetController(rig);
			rigcon->SetSkeletalMesh(sk);
			LocalEpicSkeletonSetup(rigcon);

			// bone chain
			for (auto& modelName : VRMUtil::table_ue4_vrm) {
				if (modelName.BoneVRM == "") continue;
				//if (modelName.BoneUE4 == "") continue; // commentout add as none

				// spine
				int type = 0;
				if (modelName.BoneVRM == TEXT("spine")) {
					type = 1;
				}
				if (modelName.BoneVRM == TEXT("chest") || modelName.BoneVRM == TEXT("upperChest")) {
					type = 2;
				}

				switch (type) {
				case 0:
					VRMAddRetargetChain(rigcon, *modelName.BoneVRM, *modelName.BoneUE4, *modelName.BoneUE4);
						break;
				case 1:
					if (sk->GetRefSkeleton().FindBoneIndex(TEXT("spine_05")) != INDEX_NONE) {
						VRMAddRetargetChain(rigcon, TEXT("spine"), TEXT("spine_01"), TEXT("spine_05"));
					} else {
						VRMAddRetargetChain(rigcon, TEXT("spine"), TEXT("spine_01"), TEXT("spine_03"));
					}
					break;
				default:
					break;
				}
			}
			//VRMAddRetargetChain(rigcon, TEXT("leftEye"), NAME_None, NAME_None);
			//VRMAddRetargetChain(rigcon, TEXT("rightEye"), NAME_None, NAME_None);
		}

		{
			// sub epic bone
			UIKRigDefinition* rig_epic = nullptr;
			FString name = FString(TEXT("IK_")) + sk->GetName() + TEXT("_MannequinBone");
			UPackage* pkg = CreatePackage(*(SavePackagePath + "/" + name));

			{
				auto* a = FindObject<UIKRigDefinition>(NULL, *(SavePackagePath + "/" + name + TEXT(".") + name));
				if (a != nullptr) {
					rig_epic = a;
				}
			}
			if (rig_epic == nullptr) {
				rig_epic = VRM4U_NewObject<UIKRigDefinition>(pkg, *name, RF_Public | RF_Standalone);
			}

			UIKRigController* rigcon = LocalGetController(rig_epic);
			rigcon->SetSkeletalMesh(sk);
			LocalEpicSkeletonSetup(rigcon);

			// bone chain
			for (auto& modelName : VRMUtil::table_ue4_vrm) {
				if (modelName.BoneVRM == "") continue;
				if (modelName.BoneUE4 == "") continue;
				VRMAddRetargetChain(rigcon, *modelName.BoneUE4, *modelName.BoneUE4, *modelName.BoneUE4);
			}
		}

		/*
		{ // sub ik
			UIKRigDefinition* ik = nullptr;
			{
				FString name = FString(TEXT("IK_")) + sk->GetName() + TEXT("_EpicSkeleton_IK");
				UPackage* pkg = CreatePackage(*(SavePackagePath + "/" + name));

				{
					auto* a = FindObject<UIKRigDefinition>(NULL, *(SavePackagePath + "/" + name + TEXT(".") + name));
					if (a != nullptr) {
						ik = a;
					}
				}
				if (ik == nullptr) {
					ik = VRM4U_NewObject<UIKRigDefinition>(pkg, *name, RF_Public | RF_Standalone);
				}

				UIKRigController* rigcon = UIKRigController::GetIKRigController(ik);
				rigcon->SetSkeletalMesh(sk);
				LocalEpicSkeletonSetup(rigcon);

				struct TT {
					FString s1;
					FString s2;
					FString s3;
				};
				TArray<TT> table = {
					{TEXT("pelvis"),		TEXT("spine_03"),	TEXT(""),},
					{TEXT("neck_01"),		TEXT("head"),		TEXT(""),},
					{TEXT("clavicle_l"),	TEXT("hand_l"),		TEXT("hand_l_Goal"),},
					{TEXT("clavicle_r"),	TEXT("hand_r"),		TEXT("hand_r_Goal"),},
					{TEXT("thigh_l"),		TEXT("ball_l"),		TEXT("ball_l_Goal"),},
					{TEXT("thigh_r"),		TEXT("ball_r"),		TEXT("ball_r_Goal"),},

					{TEXT("thumb_01_l"),	TEXT("thumb_03_l"),		TEXT(""),},
					{TEXT("index_01_l"),	TEXT("index_03_l"),		TEXT(""),},
					{TEXT("middle_01_l"),	TEXT("middle_03_l"),	TEXT(""),},
					{TEXT("ring_01_l"),		TEXT("ring_03_l"),		TEXT(""),},
					{TEXT("pinky_01_l"),	TEXT("pinky_03_l"),		TEXT(""),},

					{TEXT("thumb_01_r"),	TEXT("thumb_03_r"),		TEXT(""),},
					{TEXT("index_01_r"),	TEXT("index_03_r"),		TEXT(""),},
					{TEXT("middle_01_r"),	TEXT("middle_03_r"),	TEXT(""),},
					{TEXT("ring_01_r"),		TEXT("ring_03_r"),		TEXT(""),},
					{TEXT("pinky_01_r"),	TEXT("pinky_03_r"),		TEXT(""),},
				};


				for (auto& a : table) {
					rigcon->AddRetargetChain(*a.s1, *a.s1, *a.s2);
					if (a.s3 != "") {
						rigcon->SetRetargetChainGoal(*a.s1, *a.s3);
					}
				}
			}
		}
		*/

		UIKRetargeter* ikr = nullptr;
		{
			FString name = FString(TEXT("RTG_")) + sk->GetName();
			UPackage* pkg = CreatePackage(*(SavePackagePath +"/"+ name));

			{
				auto* a = FindObject<UIKRetargeter>(NULL, *(SavePackagePath + "/" + name + TEXT(".") + name));
				if (a != nullptr) {
					ikr = a;
				}
			}
			if (ikr == nullptr) {
				ikr = VRM4U_NewObject<UIKRetargeter>(pkg, *name, RF_Public | RF_Standalone);
			}
			UIKRetargeterController* c = UIKRetargeterController::GetController(ikr);
#if	UE_VERSION_OLDER_THAN(5,2,0)
			c->SetSourceIKRig(rig);
#else
			c->SetIKRig(ERetargetSourceOrTarget::Source, rig);
#endif
		}

		rig->PostEditChange();
		ikr->PostEditChange();

		outRigIK = rig;
		outIKRetargeter = ikr;
	}
#endif
#endif // editor
	return;
}


void ULoaderBPFunctionLibrary::VRMGenerateIKRetargeterPose(UObject* IKRetargeter, UObject* targetRigIK, UPoseAsset* targetPose) {
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(5,0,0)
#else

	if (targetPose == nullptr) return;

	UIKRetargeter* ikr = Cast<UIKRetargeter>(IKRetargeter);
	if (ikr == nullptr) return;

	UIKRetargeterController* c = UIKRetargeterController::GetController(ikr);
	if (c == nullptr) return;

#if UE_VERSION_OLDER_THAN(5,1,0)
	// setup A-Pose
	if (targetRigIK) {
		c->SetTargetIKRig(Cast<UIKRigDefinition>(targetRigIK));
	}

	if (c->GetAsset()->GetTargetIKRig() && targetPose) {
		//UIKRigDefinition* d = Cast<UIKRigDefinition>(targetRigIK);
		USkeletalMesh* targetSK = targetPose->GetPreviewMesh();

		UPoseAsset* pose = targetPose;
		for (int poseNum = 0; poseNum < 10; ++poseNum) {
			auto poseName = pose->GetPoseNameByIndex(poseNum);
			if (poseName == NAME_None) break;

			// add pose
			if (poseName == c->MakePoseNameUnique(poseName)) {
				c->AddRetargetPose(poseName);
			}
			c->SetCurrentRetargetPose(poseName);
			c->ResetRetargetPose(poseName);

			// A-pose
			TArray<FTransform> outTrans;
			pose->GetFullPose(poseNum, outTrans);

			auto& rsk = targetSK->GetRefSkeleton();
			for (int i = 0; i < rsk.GetRawBoneNum(); ++i) {
				if (outTrans.IsValidIndex(i) == false) {
					continue;
				}
				auto q = outTrans[i].GetRotation();
				c->SetRotationOffsetForRetargetPoseBone(targetSK->GetRefSkeleton().GetBoneName(i), q);
			}
		}
		// set A-pose as current pose
		FName poseName_A = pose->GetPoseNameByIndex(1);
		c->SetCurrentRetargetPose(poseName_A);
	}
#else
#endif

#endif
#endif // editor

}