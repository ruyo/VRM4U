// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once
#include "Engine/LatentActionManager.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VrmConvert.h"
#include "VrmUtil.h"
#include "LoaderBPFunctionLibrary.generated.h"

UENUM(BlueprintType)
enum class EPathType : uint8
{
	Absolute,
	Relative
};


USTRUCT(BlueprintType)
struct FMeshInfo_VRM4U
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector> Vertices;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector> Normals;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<uint32> Triangles;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<int32> Triangles2;


//	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
	TArray< TArray<FVector2D> > UV0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FLinearColor> VertexColors;

//	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
//		TArray<FProcMeshTangent> MeshTangents;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FVector> Tangents;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		FTransform RelativeTransform;

	TArray<bool> vertexUseFlag;
	TArray<uint32_t> vertexIndexOptTable;
	uint32_t useVertexCount = 0;
};

USTRUCT(BlueprintType)
struct FReturnedData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		bool bSuccess = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		int32 NumMeshes = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
		TArray<FMeshInfo_VRM4U> meshInfo;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ReturnedData")
	//TMap<struct aiMesh*, uint32_t> meshToIndex;
};




/**
 * 
 */
UCLASS()
class VRM4ULOADER_API ULoaderBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static bool VRMSetLoadMaterialType(EVRMImportMaterialType type);

	UFUNCTION(BlueprintCallable,Category="VRM4U", meta = (DynamicOutputParam = "OutVrmAsset"))
	static bool LoadVRMFile(const class UVrmAssetListObject *InVrmAsset, class UVrmAssetListObject *&OutVrmAsset, const FString filepath, const FImportOptionData &OptionForRuntimeLoad);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (Latent, DynamicOutputParam = "OutVrmAsset", WorldContext = "WorldContextObject", LatentInfo = "LatentInfo"))
	static void LoadVRMFileAsync(const UObject* WorldContextObject, const class UVrmAssetListObject* InVrmAsset, class UVrmAssetListObject*& OutVrmAsset, const FString filepath, const FImportOptionData& OptionForRuntimeLoad, struct FLatentActionInfo LatentInfo);


	static bool LoadVRMFileLocal(const class UVrmAssetListObject* InVrmAsset, class UVrmAssetListObject*& OutVrmAsset, const FString filepath);

	static bool LoadVRMFileFromMemoryDefaultOption(UVrmAssetListObject*& OutVrmAsset, const FString filepath, const uint8* pData, size_t dataSize);
	static bool LoadVRMFileFromMemory(const UVrmAssetListObject* InVrmAsset, UVrmAssetListObject*& OutVrmAsset, const FString filepath, const uint8* pFileData, size_t dataSize);

	static void SetImportMode(bool bImportMode, class UPackage *package);

	//static void SetCopySkeletalMeshAnimation(bool bImportMode, class UPackage *package);

	///
	UFUNCTION(BlueprintCallable,Category="VRM4U")
	static bool VRMReTransformHumanoidBone(class USkeletalMeshComponent *targetHumanoidSkeleton, const class UVrmMetaObject *meta, const class USkeletalMeshComponent *displaySkeleton);

	//static void ReTransformHumanoidBone(USkeleton *targetHumanoidSkeleton, const UVrmMetaObject *meta, const USkeleton *displaySkeleton) {

	///
	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static bool CopyPhysicsAsset(USkeletalMesh *dstMesh, const USkeletalMesh *srcMesh, bool bResetCollisionTransform=true);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static bool CopyVirtualBone(USkeletalMesh *dstMesh, const USkeletalMesh *srcMesh);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static bool CreateTailBone(USkeletalMesh *skeletalMesh, const TArray<FString> &boneName);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DynamicOutputParam = "RigIK"))
	static void VRMGenerateEpicSkeletonToHumanoidIKRig(USkeletalMesh* srcSkeletalMesh, UObject*& outRigIK, UObject*& outIKRetargeter, UObject* targetRigIK);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DynamicOutputParam = "RigIK"))
	static void VRMGenerateIKRetargeterPose(UObject * IKRetargeter, UObject* targetRigIK, UPoseAsset* targetPose);

	static bool IsValidVRM4UFile(FString filepath);
	static void GetVRMMeta(FString filepath, UVrmLicenseObject*& a, UVrm1LicenseObject*& b);
};
