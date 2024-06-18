// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "VrmPoseableMeshComponent.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class VRM4U_API UVrmPoseableMeshComponent : public UPoseableMeshComponent
{
	GENERATED_UCLASS_BODY()
	
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	bool bUseParentAsLeader = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	bool bUseDefaultMaterial = false;

	void OnRegister() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "VRM4U")
	void Init();

	virtual void RefreshBoneTransforms(FActorComponentTickFunction* TickFunction = NULL) override;

	UFUNCTION(BlueprintCallable, Category = "Components|PoseableMesh")
	void VRMCopyPoseAndMorphFromSkeletalComponent(USkeletalMeshComponent* InComponentToCopy);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void UpdateLeader();

	virtual void InitializeComponent() override;
	virtual void OnAttachmentChanged() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif

};
