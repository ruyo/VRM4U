// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/EngineVersionComparison.h"
#include "Engine/SkeletalMesh.h"

#if UE_VERSION_OLDER_THAN(5,4,0)
#else
#include "Misc/FieldAccessor.h"
#endif

#include "VrmUtil.h"
#include "VrmJson.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>

//#include "VrmConvert.generated.h"

/**
 * 
 */

struct aiScene;
class UTexture2D;
class UMaterialInterface;
class USkeletalMesh;
class UVrmAssetListObject;
class UVrmLicenseObject;
class UVrm1LicenseObject;
class UPackage;


class VRM4ULOADER_API VRMConverter {

	bool InitJSON(const uint8* pData, size_t pFileDataSize);

public:

	VrmJson jsonData;
	const aiScene* aiData = nullptr;

	char* GetMatName(int matNo) const;
	char* GetMatShaderName(int matNo) const;
	int GetMatNum() const;
	int GetMatCullMode(int matNo) const;
	int GetMatZWrite(int matNo) const;

	int GetThumbnailTextureIndex() const;

	bool GetMatParam(VRM::VRMMaterial &m, int matNo) const;


	static bool IsImportMode();
	static void SetImportMode(bool bImportMode);
	static FString NormalizeFileName(const char *str);
	static FString NormalizeFileName(const FString &str);

	static bool NormalizeBoneName(const aiScene *mScenePtr);

	bool Init(const uint8* pFileData, size_t dataSize, const aiScene*);
	bool ValidateSchema();

	bool ConvertTextureAndMaterial(UVrmAssetListObject *vrmAssetList);
	bool ConvertModel(UVrmAssetListObject *vrmAssetList);
	bool ConvertMorphTarget(UVrmAssetListObject *vrmAssetList);

	void GetVRMMeta(const aiScene *mScenePtr, UVrmLicenseObject *& a, UVrm1LicenseObject *& b);
	bool ConvertVrmFirst(UVrmAssetListObject* vrmAssetList, const uint8* pData, size_t dataSize);
	bool ConvertVrmMeta(UVrmAssetListObject *vrmAssetList, const aiScene *mScenePtr, const uint8* pData, size_t dataSize);
	bool ConvertVrmMetaPost(UVrmAssetListObject* vrmAssetList, const aiScene* mScenePtr, const uint8* pData, size_t dataSize);

	bool ConvertHumanoid(UVrmAssetListObject *vrmAssetList);
	bool ConvertRig(UVrmAssetListObject *vrmAssetList);
	bool ConvertIKRig(UVrmAssetListObject* vrmAssetList);
	bool ConvertPose(UVrmAssetListObject* vrmAssetList);

	UPackage *CreatePackageFromImportMode(UPackage *p, const FString &name);

	class VRM4ULOADER_API Options {
	public:
		static Options & Get();

		const FImportOptionData *ImportOption = nullptr;
		void SetVrmOption(const FImportOptionData *p) {
			ImportOption = p;
		}

		class USkeleton *GetSkeleton();
		bool IsSimpleRootBone() const;

		bool IsActiveBone() const;

		bool IsSkipPhysics() const;
		bool IsSkipRetargeter() const;

		bool IsSkipNoMeshBone() const;

		bool IsSkipMorphTarget() const;

		bool IsEnableMorphTargetNormal() const;

		bool IsForceOriginalMorphTargetName() const;

		bool IsRemoveBlendShapeGroupPrefix() const;

		bool IsRemoveRootBoneRotation() const;
		bool IsRemoveRootBonePosition() const;

		bool IsVRM10RemoveLocalRotation() const;
		bool IsVRM10BindToRestPose() const;

		bool IsVRM10Bindpose() const;

		bool IsForceOriginalBoneName() const;
		bool IsGenerateHumanoidRenamedMesh() const;

		bool IsGenerateIKBone() const;
		bool IsGenerateRigIK() const;

		bool IsDebugIgnoreVRMValidation() const;
		bool IsDebugOneBone() const;
		bool IsDebugNoMesh() const;
		bool IsDebugNoMaterial() const;

		bool IsMobileBone() const;

		int GetBoneWeightInfluenceNum() const;

		bool IsForceOpaque() const;
		bool IsForceTwoSided() const;

		bool IsSingleUAssetFile() const;

		bool IsDefaultGridTextureMode() const;
		bool IsBC7Mode() const;
		bool IsMipmapGenerateMode() const;

		bool IsGenerateOutlineMaterial() const;
		bool IsMergeMaterial() const;

		bool IsMergePrimitive() const;

		bool IsOptimizeMaterial() const;

		bool IsOptimizeVertex() const;

		bool IsRemoveDegenerateTriangles() const;

		void ClearModelType();

		bool IsUE5Material() const;

		bool IsVRMModel() const;
		bool IsVRM0Model() const;
		bool IsVRM10Model() const;

		void SetVRM0Model(bool bVRM);
		void SetVRM10Model(bool bVRM);

		bool IsVRMAModel() const;
		void SetVRMAModel(bool bVRMA);

		bool IsBVHModel() const;
		void SetBVHModel(bool bBVH);

		bool IsPMXModel() const;
		void SetPMXModel(bool bPMX);

		bool IsNoMesh() const;
		void SetNoMesh(bool bNoMesh);

		bool IsForceOverride() const;
		float GetModelScale() const;

		float GetAnimationTranslateScale() const;

		float GetAnimationPlayRateScale() const;

		bool IsAPoseRetarget() const;

		EVRMImportMaterialType GetMaterialType() const;
		void SetMaterialType(EVRMImportMaterialType type);
	};
};

class VRM4ULOADER_API VRMLoaderUtil {
public:
	static UTexture2D* CreateTexture(int32 InSizeX, int32 InSizeY, FString name, UPackage* package);
	static UTexture2D* CreateTextureFromImage(FString name, UPackage* package, const void* Buffer, const size_t Length, bool GenerateMip = false, bool bNormal = false, bool bNormalGreenFlip = false);

	static bool LoadImageFromMemory(const void* Buffer, const size_t Length, VRMUtil::FImportImage& OutImage);
};


class VRM4ULOADER_API VrmConvert {
public:
	VrmConvert();
	~VrmConvert();
};

extern FString VRM4U_GetPackagePath(UPackage* Outer);
extern UPackage* VRM4U_CreatePackage(UPackage* Outer, FName Name);

template< class T >
T* VRM4U_NewObject(UObject* Outer, FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr) {
	UPackage* pkg = Cast<UPackage>(Outer);
	if (VRMConverter::Options::Get().IsSingleUAssetFile() == false) {
		pkg = VRM4U_CreatePackage(pkg, Name);
	}
	decltype(auto) r = NewObject<T>(pkg, Name, Flags, Template, bCopyTransientsFromClassDefaults, InInstanceGraph);
	r->MarkPackageDirty();
	return r;
}

template< class T >
T* VRM4U_NewObject(UObject* Outer, UClass* Class, FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false, FObjectInstancingGraph* InInstanceGraph = nullptr) {
	UPackage* pkg = Cast<UPackage>(Outer);
	if (VRMConverter::Options::Get().IsSingleUAssetFile() == false) {
		pkg = VRM4U_CreatePackage(pkg, Name);
	}
	decltype(auto) r = NewObject<T>(pkg, Class, Name, Flags, Template, bCopyTransientsFromClassDefaults, InInstanceGraph);
	r->MarkPackageDirty();
	return r;
}

template< class T >
T* VRM4U_DuplicateObject(const T *src, UPackage* Outer, FName Name) {
	UPackage* pkg = Outer;
	if (VRMConverter::Options::Get().IsSingleUAssetFile() == false) {
		pkg = VRM4U_CreatePackage(Outer, Name);
	}
	decltype(auto) r = DuplicateObject<T>(src, pkg, Name);
	r->MarkPackageDirty();
	return r;
}

#if	UE_VERSION_OLDER_THAN(5,4,0)
extern UObject* VRM4U_StaticDuplicateObject(UObject const* SourceObject, UObject* DestOuter, const FName DestName = NAME_None, EObjectFlags FlagMask = RF_AllFlags, UClass* DestClass = nullptr, EDuplicateMode::Type DuplicateMode = EDuplicateMode::Normal, EInternalObjectFlags InternalFlagsMask = EInternalObjectFlags::AllFlags);
#else
extern UObject* VRM4U_StaticDuplicateObject(UObject const* SourceObject, UObject* DestOuter, const FName DestName = NAME_None, EObjectFlags FlagMask = RF_AllFlags, UClass* DestClass = nullptr, EDuplicateMode::Type DuplicateMode = EDuplicateMode::Normal, EInternalObjectFlags InternalFlagsMask = EInternalObjectFlags_AllFlags);
#endif


#if	UE_VERSION_OLDER_THAN(5,0,0)
template<typename T>
FTexturePlatformData* GetPlatformData(T* t) {
	return t->PlatformData;
}
template<typename T, typename U>
void SetPlatformData(T* t, U* u) {
	t->PlatformData = u;
}
#else
template<typename T>
TFieldPtrAccessor<FTexturePlatformData> GetPlatformData(T* t) {
	return t->GetPlatformData();
}
template<typename T, typename U>
void SetPlatformData(T* t, U* u) {
	t->SetPlatformData(u);
}
#endif

