// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/EngineVersionComparison.h"

#include "VrmEditorBPFunctionLibrary.generated.h"

class UTextureRenderTarget2D;
class UMaterialInstanceConstant;
class UAnimationAsset;
class USkeleton;

/**
 * 
 */
UCLASS()
class VRM4UEDITOR_API UVrmEditorBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	//UFUNCTION(BlueprintCallable,Category="VRM4U", meta = (DevelopmentOnly))
	//static bool VRMBakeAnim(const USkeletalMeshComponent *skc, const FString &FilePath, const FString &AssetFileName);


	//UFUNCTION(BlueprintCallable,Category="VRM4U")
	//static void VRMTransMatrix(const FTransform &transform, TArray<FLinearColor> &matrix, TArray<FLinearColor> &matrix_inv);

	//UFUNCTION(BlueprintPure, Category = "VRM4U")
	//static void VRMGetMorphTargetList(const USkeletalMesh *target, TArray<FString> &morphTargetList);


	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static int EvaluateCurvesFromSequence(const UMovieSceneSequence* Seq, float FrameNo, TArray<FString>& names, TArray<float>& curves);
};
