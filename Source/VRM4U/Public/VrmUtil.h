// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "UObject/ObjectMacros.h"
#include "Engine/Texture.h"

#if	UE_VERSION_OLDER_THAN(5,1,0)
#else
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/SkinnedAsset.h"
#endif

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "VrmUtil.generated.h"

class USkeleton;
struct FReferenceSkeleton;
class UMorphTarget;
class UPhysicsAsset;
struct FSkeletalMaterial;

// skeleton begin

template<typename T>
USkeleton* VRMGetSkeleton(T* t){
	if (t == nullptr) return nullptr;
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->Skeleton;
#else
	return t->GetSkeleton();
#endif
}

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
template<typename T>
USkeleton* VRMGetSkeleton(TObjectPtr<T> t) {
	return t->GetSkeleton();
}
#endif

template<typename T>
const USkeleton* VRMGetSkeleton(const T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->Skeleton;
#else
	return t->GetSkeleton();
#endif
}

template<typename T>
void VRMSetSkeleton(T* t, USkeleton *sk) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	t->Skeleton = sk;
#else
	t->SetSkeleton(sk);
#endif
}

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
template<typename T>
void VRMSetSkeleton(TObjectPtr<T> t, USkeleton* sk) {
	t->SetSkeleton(sk);
}
#endif

//skeleton end

// refskeleton begin

template<typename T>
FReferenceSkeleton& VRMGetRefSkeleton(T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->RefSkeleton;
#else
	return t->GetRefSkeleton();
#endif
}

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
template<typename T>
FReferenceSkeleton& VRMGetRefSkeleton(TObjectPtr<T> t) {
	return t->GetRefSkeleton();
}
#endif

template<typename T>
const FReferenceSkeleton& VRMGetRefSkeleton(const T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->RefSkeleton;
#else
	return t->GetRefSkeleton();
#endif
}

template<typename T>
void VRMSetRefSkeleton(T* t, const FReferenceSkeleton& refsk) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	t->RefSkeleton = refsk;
#else
	t->SetRefSkeleton(refsk);
#endif
}

#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
template<typename T>
void VRMSetRefSkeleton(TObjectPtr<T> t, const FReferenceSkeleton& refsk) {
	t->SetRefSkeleton(refsk);
}
#endif

//refskeleton end

// morph targets
#if	UE_VERSION_OLDER_THAN(5,4,0)
template<typename T>
TArray<UMorphTarget*>& VRMGetMorphTargets(T* t) {
#else
template<typename T>
TArray<TObjectPtr<UMorphTarget>>& VRMGetMorphTargets(T * t) {
#endif
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->MorphTargets;
#else
	return t->GetMorphTargets();
#endif
}


template<typename T>
const TArray<UMorphTarget*>& VRMGetMorphTargets(const T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->MorphTargets;
#else
	return t->GetMorphTargets();
#endif
}

// materials

template<typename T>
TArray<FSkeletalMaterial>& VRMGetMaterials(T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->Materials;
#else
	return t->GetMaterials();
#endif
}

// physics

template<typename T>
void VRMSetPhysicsAsset(T* t, UPhysicsAsset *pa) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	t->PhysicsAsset = pa;
#else
	t->SetPhysicsAsset(pa);
#endif
}


template<typename T>
UPhysicsAsset* VRMGetPhysicsAsset(T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->PhysicsAsset;
#else
	return t->GetPhysicsAsset();
#endif
}

// nodemappingdata
#if	UE_VERSION_OLDER_THAN(5,4,0)
template<typename T>
TArray<class UNodeMappingContainer*>& VRMGetNodeMappingData(T* t) {
#else
template<typename T>
TArray<TObjectPtr<class UNodeMappingContainer>>& VRMGetNodeMappingData(T * t) {
#endif
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->NodeMappingData;
#else
	return t->GetNodeMappingData();
#endif
}

template<typename T>
const TArray<class UNodeMappingContainer*>& VRMGetNodeMappingData(const T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->NodeMappingData;
#else
	return t->GetNodeMappingData();
#endif
}

// retargetbase


template<typename T>
void VRMSetRetargetBasePose(T* t, TArray<FTransform>& pose) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	t->RetargetBasePose = pose;
#elif UE_VERSION_OLDER_THAN(5,3,0)
	t->SetRetargetBasePose(pose);
#else
	// no old retarget pose.
#endif
}


template<typename T>
TArray<FTransform>& VRMGetRetargetBasePose(T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->RetargetBasePose;
#else
	return t->GetRetargetBasePose();
#endif
}

template<typename T>
const TArray<FTransform>& VRMGetRetargetBasePose(const T* t) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	return t->RetargetBasePose;
#else
	return t->GetRetargetBasePose();
#endif
}


// key
template<typename T>
void VRMSetUseLegacyMeshDerivedDataKey(T* t, bool b) {
#if	UE_VERSION_OLDER_THAN(4,27,0)
	t->UseLegacyMeshDerivedDataKey = b;
#elif UE_VERSION_OLDER_THAN(5,4,0)
	t->SetUseLegacyMeshDerivedDataKey(b);
#else
	// ue5.4 no action
#endif
}


#if	UE_VERSION_OLDER_THAN(5,0,0)
#else
VRM4U_API void VRMAddRetargetChain(class UIKRigController* con, FName name, FName begin, FName end);
#endif


//template<typename T>
FORCEINLINE  USkeletalMesh *VRMGetSkeletalMeshAsset(USkeletalMeshComponent* t) {
#if	UE_VERSION_OLDER_THAN(5,1,0)
	return t->SkeletalMesh;
#else
	return (t->GetSkeletalMeshAsset());
#endif
}
FORCEINLINE  const USkeletalMesh* VRMGetSkeletalMeshAsset(const USkeletalMeshComponent* t) {
#if	UE_VERSION_OLDER_THAN(5,1,0)
	return t->SkeletalMesh;
#else
	return (t->GetSkeletalMeshAsset());
#endif
}
FORCEINLINE  USkeletalMesh *VRMGetSkeletalMeshAsset(TWeakObjectPtr<USkeletalMeshComponent> t) {
#if	UE_VERSION_OLDER_THAN(5,1,0)
	return t->SkeletalMesh;
#else
	return (t->GetSkeletalMeshAsset());
#endif
}
FORCEINLINE  const USkeletalMesh* VRMGetSkeletalMeshAsset(TWeakObjectPtr<const USkeletalMeshComponent> t) {
#if	UE_VERSION_OLDER_THAN(5,1,0)
	return t->SkeletalMesh;
#else
	return (t->GetSkeletalMeshAsset());
#endif
}

//template<>
FORCEINLINE  USkeletalMesh *VRMGetSkinnedAsset(USkinnedMeshComponent* t) {
#if	UE_VERSION_OLDER_THAN(5,1,0)
	return t->SkeletalMesh;
#else
	return Cast<USkeletalMesh>(t->GetSkinnedAsset());
#endif
}
//template<>
FORCEINLINE  USkeletalMesh *VRMGetSkinnedAsset(const USkinnedMeshComponent* t) {
#if	UE_VERSION_OLDER_THAN(5,1,0)
	return t->SkeletalMesh;
#else
	return Cast<USkeletalMesh>(t->GetSkinnedAsset());
#endif
}


// 
//


UENUM(BlueprintType)
enum class EVRMImportMaterialType : uint8
{
	VRMIMT_Auto				UMETA(DisplayName = "Auto(MToon Unlit)"),
	VRMIMT_MToon			UMETA(DisplayName = "MToon Lit"),
	VRMIMT_MToonUnlit		UMETA(DisplayName = "MToon Unlit"),
	VRMIMT_SSS				UMETA(DisplayName = "Subsurface"),
	VRMIMT_SSSProfile		UMETA(DisplayName = "Subsurface Profile"),
	VRMIMT_Unlit			UMETA(DisplayName = "Unlit"),
	VRMIMT_glTF				UMETA(DisplayName = "PBR(glTF2)"),
	VRMIMT_UEFNLit			UMETA(DisplayName = "UEFN(Lit)"),
	VRMIMT_UEFNUnlit		UMETA(DisplayName = "UEFN(Unlit)"),
	VRMIMT_UEFNSSSProfile	UMETA(DisplayName = "UEFN(SSSProfile)"),
	VRMIMT_Custom			UMETA(DisplayName = "Custom"),

	VRMIMT_MAX,
};

UENUM(BlueprintType)
enum class EVRMImportTextureCompressType : uint8
{
	VRMITC_Auto				UMETA(DisplayName = "Auto(DXT1)"),
	VRMITC_DXT1				UMETA(DisplayName = "DXT1"),
	VRMITC_BC7				UMETA(DisplayName = "BC7"),
	VRMITC_NoCompression	UMETA(DisplayName = "NoCompression"),

	VRMITC_MAX,
};

USTRUCT(BlueprintType)
struct VRM4U_API FImportOptionData {
public:
	GENERATED_BODY()

#if UE_VERSION_OLDER_THAN(4,26,0)
	static const bool VRM4U_UseBC7 = false;
#else
	static const bool VRM4U_UseBC7 = true;
#endif

#if UE_VERSION_OLDER_THAN(5,2,0)
	static const bool VRM4U_UseUE5Mat = false;
#else
	static const bool VRM4U_UseUE5Mat = true;
#endif


	void init();

	bool bUEFN = false;

	bool bAPoseRetarget = true;

	bool bMobileBone = false;

	int BoneWeightInfluenceNum = 8;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	EVRMImportMaterialType MaterialType = EVRMImportMaterialType::VRMIMT_Auto;

	bool bForceOverride = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	float ModelScale = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	float AnimationTranslateScale = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	float PlayRateScale = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bRemoveRootBoneRotation = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bRemoveRootBonePosition = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bVrm10RemoveLocalRotation = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bVrm10UseBindToRestPose = true;

	bool bVrm10Bindpose = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bForceOriginalBoneName = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bGenerateHumanoidRenamedMesh = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bGenerateIKBone = false;

	bool bGenerateRigIK = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bSkipPhysics = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bSkipRetargeter = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bSkipMorphTarget = false;

	bool bEnableMorphTargetNormal = false;
	
	bool bForceOriginalMorphTargetName = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bRemoveBlendShapeGroupPrefix = false;
	

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bForceOpaque= false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bForceTwoSided = false;

	bool bSingleUAssetFile = false;

	bool bDefaultGridTextureMode = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bBC7Mode = VRM4U_UseBC7;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bMipmapGenerateMode = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bUseUE5Material = VRM4U_UseUE5Mat;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bGenerateOutlineMaterial = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bMergeMaterial = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bMergePrimitive = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bOptimizeVertex = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bRemoveDegenerateTriangles = false;

	bool bSimpleRoot = true;

	bool bActiveBone = true;

	bool bSkipNoMeshBone = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bIgnoreVRMValidation = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bDebugOneBone = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bDebugNoMesh = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "VRM4U")
	bool bDebugNoMaterial = false;

	UPROPERTY()
	class USkeleton* Skeleton = nullptr;

};



class VRM4U_API VRMUtil {
	//GENERATED_UCLASS_BODY()
public:
	struct VRMBoneTable {
		FString BoneUE4;
		FString BoneVRM;
	};
	static const TArray<VRMBoneTable> table_ue4_vrm;
	static const TArray<VRMBoneTable> table_ue4_pmx;
	static bool GetReplacedPMXBone(FString &replaced, const FString &base);

	static const TArray<FString> vrm_humanoid_bone_list;
	static const TArray<FName> vrm_humanoid_bone_list_name;

	static const TArray<FString> vrm_humanoid_parent_list;

	static const TArray<FString> ue4_humanoid_bone_list;
	static const TArray<FName> ue4_humanoid_bone_list_name;

	struct FImportImage
	{
#if	UE_VERSION_OLDER_THAN(4,23,0)
		TArray<uint8> RawData;
#else
		TArray64<uint8> RawData;
#endif
		ETextureSourceFormat Format = TSF_Invalid;
		TextureCompressionSettings CompressionSettings = TC_Default;
		int32 NumMips;
		int32 SizeX = 0;
		int32 SizeY = 0;
		bool SRGB = true;

		static int32 GetBytesPerPixel(ETextureSourceFormat Format)
		{
			int32 BytesPerPixel = 0;
			switch (Format)
			{
			case TSF_G8:		BytesPerPixel = 1; break;
#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
			case TSF_G16:		BytesPerPixel = 2; break;
#endif
			case TSF_BGRA8:		BytesPerPixel = 4; break;
			case TSF_BGRE8:		BytesPerPixel = 4; break;
			case TSF_RGBA16:	BytesPerPixel = 8; break;
			case TSF_RGBA16F:	BytesPerPixel = 8; break;
			default:			BytesPerPixel = 0; break;
			}
			return BytesPerPixel;
		}


		void Init2DWithParams(int32 InSizeX, int32 InSizeY, ETextureSourceFormat InFormat, bool InSRGB);
		void Init2DWithOneMip(int32 InSizeX, int32 InSizeY, ETextureSourceFormat InFormat, const void* InData = nullptr) {
			SizeX = InSizeX;
			SizeY = InSizeY;
			NumMips = 1;
			Format = InFormat;
			RawData.AddUninitialized((int64)SizeX * SizeY * GetBytesPerPixel(Format));
			if (InData)
			{
				FMemory::Memcpy(RawData.GetData(), InData, RawData.Num());
			}
		}
		void Init2DWithMips(int32 InSizeX, int32 InSizeY, int32 InNumMips, ETextureSourceFormat InFormat, const void* InData = nullptr);

		int64 GetMipSize(int32 InMipIndex) const;
		void* GetMipData(int32 InMipIndex);
	};


	static bool IsNoSafeName(const FString& str);
	static FString GetSafeNewName(const FString& str);
	static FString MakeName(const FString& str, bool IsJoint = false);
	static FName GetSanitizedName(const FString& str);

	static int32 GetDirectChildBones(FReferenceSkeleton& refs, int32 ParentBoneIndex, TArray<int32>& Children);

	static class UVrmAssetListObject* GetAssetListObjectAny(const UObject* obj);
	static class UVrmAssetListObject* GetAssetListObject(const USkeletalMesh *);

	static void CloseEditorWindowByFolderPath(const UObject* obj);

	static int GetChildBone(const USkeleton* skeleton, const int32 ParentBoneIndex, bool recursive, TArray<int32>& Children);
	static int GetChildBone(const USkeleton* skeleton, const int32 ParentBoneIndex, TArray<int32>& Children) {
		return GetChildBone(skeleton, ParentBoneIndex, true, Children);
	}

};

class VRM4U_API VRMRetargetData {
public:

	struct RetargetParts {
		FString BoneUE4;
		FString BoneVRM;
		FString BoneModel;

		FRotator rot;
		bool operator==(const RetargetParts& a) const{
			return BoneUE4.Compare(a.BoneUE4, ESearchCase::IgnoreCase) == 0;
		}
	};
	class UVrmAssetListObject* vrmAssetList = nullptr;
	TArray<RetargetParts> retargetTable;

	void Setup(class UVrmAssetListObject* vrmAssetList, bool bVRM, bool bBVH, bool bPMX);
	void UpdateBoneName();
	void Remove(FString BoneUE4);
};


