// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "Misc/CoreDelegates.h"

#include "VrmEditorEventComponent.generated.h"

UENUM(BlueprintType)
enum class EVRM4U_PIEEvent : uint8
{
	PreBeginPIE,
	BeginPIE,
	PrePIEEnded,
	PostPIEStarted,
	EndPIE,
	PausePIE,
	ResumePIE,
	SingleStepPIE,
	OnPreSwitchBeginPIEAndSIE,
	OnSwitchBeginPIEAndSIE,
};


/**
 * 
 */
UCLASS(meta=(BlueprintSpawnableComponent))
class VRM4U_API UVrmEditorEventComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
	
public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVrmSelectionChangedEventDelegate, bool, dummy);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVrmSelectionObjectEventDelegate, bool, dummy);

	UPROPERTY(BlueprintAssignable)
	FVrmSelectionChangedEventDelegate OnSelectionChange;
	UPROPERTY(BlueprintAssignable)
	FVrmSelectionObjectEventDelegate OnSelectionObject;
	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetSelectCheck(bool bCheckOn);

	//
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVrmPIEEventDelegate, EVRM4U_PIEEvent, dummy);

	UPROPERTY(BlueprintAssignable)
	FVrmPIEEventDelegate OnPIEEvent;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetPIEEventCheck(bool bCheckOn);

	//

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVrmGlobalTimeChangeEventDelegate, float, CurrentTime);

	UPROPERTY(BlueprintAssignable)
	FVrmGlobalTimeChangeEventDelegate OnGlobalTimeChange;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetGlobalTimeCheck(bool bCheckOn);

public:
	void OnRegister() override;
	void OnUnregister() override;

private:

	void OnSelectionObjectFunc(UObject *obj);
	void OnSelectionChangeFunc(UObject *obj);

	void OnGlobalTimeChangeFunc();

	/*
	PreBeginPIE,
		BeginPIE,
		PrePIEEnded,
		PostPIEStarted,
		EndPIE,
		PausePIE,
		ResumePIE,
		SingleStepPIE,
		OnPreSwitchBeginPIEAndSIE,
		OnSwitchBeginPIEAndSIE,
*/		
	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);
	void OnPIEEventFunc(EVRM4U_PIEEvent e);

	FDelegateHandle handle;
};
