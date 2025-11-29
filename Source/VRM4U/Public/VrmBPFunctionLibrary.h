// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/EngineVersionComparison.h"
#include "Engine/Scene.h"
#include "LiveLinkTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "VrmUtil.h"

#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
#include "AssetRegistry/AssetData.h"
#endif

#if	UE_VERSION_OLDER_THAN(4,20,0)
struct FCameraTrackingFocusSettings {
	int dummy;
};
#elif UE_VERSION_OLDER_THAN(5,3,0)
#include "CinematicCamera/Public/CineCameraComponent.h"
#else
#include "CineCameraComponent.h"
#endif

#include "VrmBPFunctionLibrary.generated.h"

class UTextureRenderTarget2D;
class UMaterialInstanceConstant;
class UAnimationAsset;
class USkeleton;

#if	UE_VERSION_OLDER_THAN(4,20,0)
struct FCameraFilmbackSettings {
public:
	int dummy = 0;
};
#endif

UENUM(BlueprintType)
enum class EVRMWidgetMode : uint8
{
	WM_Translate,
	WM_Rotate,
	WM_Scale,
};


/**
 * 
 */
UCLASS()
class VRM4U_API UVrmBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable,Category="VRM4U")
	static void VRMTransMatrix(const FTransform &transform, TArray<FLinearColor> &matrix, TArray<FLinearColor> &matrix_inv);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetMorphTargetList(const USkeletalMesh *target, TArray<FString> &morphTargetList);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static bool VRMGetRefBoneTransform(const USkeleton *target, const FName boneName, FTransform &boneTransform);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetHumanoidBoneNameList(TArray<FString> &BoneNameListString, TArray<FName> &BoneNameListName);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetEpicSkeletonBoneNameList(TArray<FString>& BoneNameListString, TArray<FName>& BoneNameListName);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMGetEpicSkeletonToHumanoid(TMap<FString, FString>& table);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static bool VRMGetHumanoidParentBone(const FName boneName, FName &parentBoneName);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMInitAnim(USkeletalMeshComponent *target);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMUpdateRefPose(USkeletalMeshComponent* target, bool bForceAlignGlobal, bool bForceUE4Humanoid);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetMaterialPropertyOverrides(const UMaterialInterface *Material, TEnumAsByte<EBlendMode> &BlendMode, TEnumAsByte<EMaterialShadingModel> &ShadingModel, bool &IsTwoSided, bool &IsMasked);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetMobileMode(bool &IsMobile, bool &IsAndroid, bool &IsIOS);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject", UnsafeDuringActorConstruction = "true"))
	static void VRMDrawMaterialToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMUpdateTextureProperty(UTexture* Texture);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialParent(UMaterialInstanceConstant *dst, UMaterialInterface* NewParent, USkeletalMesh *UseSkeletalMesh);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialTexture(UMaterialInstanceConstant *dst, FName texName, UTexture *texture);

	// mat begin
	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialSkipEditChange(UMaterialInstanceConstant *material, USkeletalMesh *UseSkeletalMesh, bool bSkip);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialShadingModel(UMaterialInstanceConstant *material, EMaterialShadingModel ShadingModel, bool bOverride, bool bForce);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialBlendMode(UMaterialInstanceConstant *material, EBlendMode BlendMode, bool bOverride, bool bForce);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialScalarParameter(UMaterialInstanceConstant *material, FName paramName, float param, bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialVectorParameter(UMaterialInstanceConstant *material, FName paramName, FLinearColor param, bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMChangeMaterialStaticSwitch(UMaterialInstanceConstant *material, FName paramName, bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DevelopmentOnly))
	static void VRMGetMaterialStaticSwitch(UMaterialInstance* material, FName paramName, bool &bHasParam, bool &bEnable);

	// mat end

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static UObject* VRMDuplicateAsset(UObject *src, FString name, UObject *thisOwner);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMSetMaterial(USkeletalMesh *target, int no, UMaterialInterface *material);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMSetImportedBounds(USkeletalMesh *target, FVector min, FVector max);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static bool VRMGetAssetsByPackageName(FName PackageName, TArray<FAssetData>& OutAssetData, bool bIncludeOnlyOnDiskAssets = false);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMSetIsDirty(UObject *obj);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static UTextureRenderTarget2D* VRMCreateRenderTarget2D(UObject* WorldContextObject, int32 Width = 256, int32 Height = 256, ETextureRenderTargetFormat Format = RTF_RGBA16f, FLinearColor ClearColor = FLinearColor::Black);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static bool VRMRenderingThreadEnable(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static int VRMGetMeshSectionNum(const USkeletalMesh* mesh);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static bool VRMRemoveMeshSection(USkeletalMesh* mesh, int LODIndex, int SectionIndex);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static bool VRMGetShadowEnable(const USkeletalMesh *mesh, int MaterialIndex);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetLightingChannelPrim(UPrimitiveComponent *prim, bool channel0, bool channel1, bool channel2);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetLightingChannelLight(ULightComponent *light, bool channel0, bool channel1, bool channel2);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetCastRaytracedShadow(ULightComponent *light, bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetShadowSlopeBias(ULightComponent *light, float bias=0.5f);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetSpecularScale(ULightComponent* light, float scale=1.0f);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetPostProcessSettingAO(FPostProcessSettings& OutSettings, const FPostProcessSettings& InSettings, bool bOverride, float AOIntensity, float Radius, bool bRayTracing = true);

	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (DynamicOutputParam = "filmback"))
	static bool VRMGetCameraFilmback(UCineCameraComponent *CineCamera, FCameraFilmbackSettings &filmback);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static bool VRMSetCameraFilmback(UCineCameraComponent *CineCamera, const FCameraFilmbackSettings &filmback);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static bool VRMSetPostProcessSettingFromCineCamera(FPostProcessSettings &OutSettings, const FPostProcessSettings &InSettings, const UCineCameraComponent *CineCamera);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetPostProcessToneCurveAmount(FPostProcessSettings& OutSettings, const FPostProcessSettings& InSettings, bool bOverride, float Amount);

	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (DynamicOutputParam = "Settings"))
	static void VRMMakeCameraTrackingFocusSettings(AActor *ActorToTrack, FVector RelativeOffset, bool bDrawDebugTrackingFocusPoint, FCameraTrackingFocusSettings &Settings);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DynamicOutputParam = "Settings"))
	static void VRMSetActorLabel(AActor *Actor, const FString& NewActorLabel);

	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "transform"))
	static void VRMGetCameraTransform(const UObject* WorldContextObject, int32 PlayerIndex, bool bGameOnly, FTransform &transform, float &fovDegree);

	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMGetPlayMode(bool &bPlay, bool &bSIE, bool &bIsEditor);

	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "skeleton"))
	static bool VRMGetAnimationAssetData(const UAnimationAsset *animAsset, USkeleton *&skeleton);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMExecuteConsoleCommand(UObject* WorldContextObject, const FString &cmd);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static bool VRMSetTransparentWindow(bool bEnable, FLinearColor crKey);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static bool VRMAddTickPrerequisite(UActorComponent *dst, UActorComponent *src, bool bRemove);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMAllowTranslucentSelection(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject"))
	static void VRMSetWidgetMode(const EVRMWidgetMode mode);

	//UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DynamicOutputParam = "PropertyNames|PropertyValues|FrameNo"))
	//static bool VRMLiveLinkEvaluate(const FName Subject, TArray<FName> &PropertyNames, TArray<float> &PropertyValues, int &FrameNo);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (DevelopmentOnly))
	static bool VRMBakeAnim(const USkeletalMeshComponent* skc, const FString& FilePath, const FString& AssetFileName);

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "UObjectClass", DynamicOutputParam = "OutActors"))
	static void VRMGetAllActorsHasSceneComponent(const UObject* WorldContextObject, TSubclassOf<UObject> UObjectClass, TArray<AActor*>& OutActors);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetUEVersion(int& major, int& minor, int& patch) {
		major = ENGINE_MAJOR_VERSION;
		minor = ENGINE_MINOR_VERSION;
		patch = ENGINE_PATCH_VERSION;
	}

	UFUNCTION(BlueprintCallable, Category = "VRM4U", meta = (WorldContext = "WorldContextObject", DynamicOutputParam = "RigNodeName"))
	static void VRMGetRigNodeNameFromBoneName(const USkeleton *skeleton, const FName &boneName, FName &RigNodeName);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMSetPerBoneMotionBlur(USkinnedMeshComponent* SkinnedMesh, bool bUsePerBoneMotionBlur);
	
	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMGetIKRigDefinition(UObject *retargeter, UObject *& src, UObject *& target);
	//static void GetIKRigDefinition(UIKRetargeter, UIKRigDefinition * &src, UIKRigDefinition * &target)

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static void VRMGetPreviewMesh(UObject* target, USkeletalMesh*& mesh);

	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (DynamicOutputParam = "skeletalmesh"))
	static void VRMGetSkeletalMeshFromSkinnedMeshComponent(const USkinnedMeshComponent* target, USkeletalMesh*& skeletalmesh);

	UFUNCTION(BlueprintPure, Category = "VRM4U", meta = (DynamicOutputParam = "AssetName"))
	static void VRMGetTopLevelAssetName(const FAssetData& target, FName& AssetName);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static UVrmAssetListObject* VRMGetVrmAssetListObjectFromAsset(const UObject *Asset);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static bool VRMIsMovieRendering();

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static bool VRMIsTemporaryObject(const UObject *obj);

	UFUNCTION(BlueprintPure, Category = "VRM4U")
	static bool VRMIsEditorPreviewObject(const UObject* obj);

};


