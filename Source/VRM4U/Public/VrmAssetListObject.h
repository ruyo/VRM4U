// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "VrmImportMaterialSet.h"
#include "VrmUtil.h"
#include "VrmAssetListObject.generated.h"

class UMaterialInterface;
class UTexture2D;
class USkeletalMesh;
class USkeleton;
class UVrmMetaObject;
class UVrmLicenseObject;
class UVrm1LicenseObject;
struct FReturnedData;
class UNodeMappingContainer;

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VRM4U_API UVrmAssetListObject : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	void CopyMember(UVrmAssetListObject *dst) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InOption")
	bool bAssetSave = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InOption")
	bool bSkipMorphTarget=false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* MtoonLitSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* MtoonUnlitSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* SSSSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* SSSProfileSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* UnlitSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* GLTFSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* UEFNUnlitSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* UEFNLitSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* UEFNSSSProfileSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UVrmImportMaterialSet* CustomSet;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Lit")
	UMaterialInterface* BaseMToonLitOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Lit")
	UMaterialInterface* BaseMToonLitTranslucentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Lit")
	UMaterialInterface* OptMToonLitOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Lit")
	UMaterialInterface* OptMToonLitOpaqueTwoSidedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Lit")
	UMaterialInterface* OptMToonLitTranslucentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Lit")
	UMaterialInterface* OptMToonLitTranslucentTwoSidedMaterial;

	// unlit

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_unlit")
	UMaterialInterface* BaseMToonUnlitOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_unlit")
	UMaterialInterface* BaseMToonUnlitTranslucentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Unlit")
	UMaterialInterface* OptMToonUnlitOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Unlit")
	UMaterialInterface* OptMToonUnlitOpaqueTwoSidedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Unlit")
	UMaterialInterface* OptMToonUnlitTranslucentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon_Unlit")
	UMaterialInterface* OptMToonUnlitTranslucentTwoSidedMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial_MToon")
	UMaterialInterface* OptMToonOutlineMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UMaterialInterface* BaseUnlitOpaqueMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UMaterialInterface* BaseUnlitTranslucentMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InMaterial")
	UMaterialInterface* BasePBROpaqueMaterial;

	//

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	//USkeleton* BaseSkeleton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	USkeletalMesh* BaseSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UVrmMetaObject* VrmMetaObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UVrmMetaObject* VrmHumanoidMetaObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UVrmMetaObject* VrmMannequinMetaObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UVrmLicenseObject* VrmLicenseObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UVrm1LicenseObject* Vrm1LicenseObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<UTexture2D*> Textures;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UTexture2D* SmallThumbnailTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UObject* SSSProfile;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	EVRMImportMaterialType ImportMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<UMaterialInterface*> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<UMaterialInterface*> OutlineMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UNodeMappingContainer *HumanoidRig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UPoseAsset *PoseBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	UPoseAsset *PoseFace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<bool> MaterialFlag_Translucent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<bool> MaterialFlag_TwoSided;

	// no uproperty
	TArray<bool> MaterialFlag_Opaque;
	TMap<FString, FTransform> Pose_bind;
	TMap<FString, FTransform> Pose_tpose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	FTransform model_root_transform;

	//TArray<FTransform> poseGlobal_bindpose;	// bone
	//TArray<FTransform> poseGlobal_tpose;	// node


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	EVRMImportTextureCompressType Texture_CompressType = EVRMImportTextureCompressType::VRMITC_Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Out")
	TArray<EVRMImportTextureCompressType> Texture_CompressTypeList;

	UPROPERTY()
	UPackage *Package;

	UPROPERTY()
	UVrmAssetListObject* ReimportBase;

	UPROPERTY()
	FString FileFullPathName;

	UPROPERTY()
	FString OrigFileName;

	UPROPERTY()
	FString BaseFileName;

	UPROPERTY()
	USkeletalMesh* UE4SkeletalMesh;

	UPROPERTY()
	USkeletalMesh* HumanoidSkeletalMesh;

	TMap<int, int> MaterialMergeTable;

	TMap<FString, FString> MaterialNameOrigToAsset;
	TMap<FString, int> MaterialNameAssetToMatNo;

	TArray<bool> MaterialHasAlphaCutoff;

	TArray<bool> MaterialHasMToon;

	TSharedPtr<FReturnedData> MeshReturnedData;
	//FReturnedData *Result;

#if WITH_EDITORONLY_DATA

//#if	UE_VERSION_OLDER_THAN(5,0,0)
//	typedef LocalImportData UAssetImportData;
//#else
//	typedef LocalImportData TObjectPtr<class UAssetImportData>
//#endif

	// VRM4U_PRECOMPILE_500
	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	class UAssetImportData *AssetImportData = nullptr;
	//TObjectPtr<class UAssetImportData> AssetImportData;

#if	UE_VERSION_OLDER_THAN(5,0,0)
	//class UAssetImportData* AssetImportData = nullptr;
#else
	//TObjectPtr<class UAssetImportData> AssetImportData;
#endif

#endif


#if WITH_EDITOR


	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	// Import data for this 
	void WaitUntilAsyncPropertyReleased() const;


	class UAssetImportData* GetAssetImportData() const
	{
		//WaitUntilAsyncPropertyReleased(ESkeletalMeshAsyncProperties::AssetImportData);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			return AssetImportData;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	void SetAssetImportData(class UAssetImportData* InAssetImportData)
	{
		//WaitUntilAsyncPropertyReleased(ESkeletalMeshAsyncProperties::AssetImportData);
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			AssetImportData = InAssetImportData;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
#endif

};
