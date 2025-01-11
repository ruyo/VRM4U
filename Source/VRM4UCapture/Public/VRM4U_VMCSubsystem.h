// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/EngineVersionComparison.h"
#include "UObject/StrongObjectPtr.h"

#include "OSCServer.h"
#include "VrmVMCObject.h"

#include "VRM4U_VMCSubsystem.generated.h"


#if	UE_VERSION_OLDER_THAN(4,22,0)

//Couldn't find parent type for 'VRM4U_AnimSubsystem' named 'UEngineSubsystem'
#error "please remove VRM4U_AnimSubsystem.h/cpp  for <=UE4.21"

#endif


UCLASS()
class VRM4UCAPTURE_API UVRM4U_VMCSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = VRM4U)
	bool CreateVMCServer(const FString ServerAddress, int port);

	void ClearData(const FString ServerAddress, int port);

	UFUNCTION(BlueprintCallable, Category = VRM4U)
	void DestroyVMCServer(const FString ServerAddress, int port);

	UFUNCTION(BlueprintCallable, Category = VRM4U)
	void DestroyVMCServerAll();

	UVrmVMCObject* FindOrAddServer(const FString ServerAddress, int port);

	TArray< TStrongObjectPtr<UVrmVMCObject> > VMCObjectList;

	bool CopyVMCData(FVMCData& DstData, FString ServerAddress, int port);

	UFUNCTION(BlueprintCallable, Category = VRM4U)
	bool GetVMCData(TMap<FString, FTransform> &BoneData, TMap<FString, float> &CurveData, FString ServerAddress, int port);


	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
};
