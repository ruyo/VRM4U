// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Misc/EngineVersionComparison.h"
#include "UObject/StrongObjectPtr.h"

#include "OSCServer.h"

#include "VRM4U_VMCSubsystem.generated.h"


#if	UE_VERSION_OLDER_THAN(4,22,0)

//Couldn't find parent type for 'VRM4U_AnimSubsystem' named 'UEngineSubsystem'
#error "please remove VRM4U_AnimSubsystem.h/cpp  for <=UE4.21"

#endif

class UOSCServer; 
struct FOSCAddress;
struct FOSCMessage;

struct FVMCData {
	FString ServerAddress = "";
	int Port = 0;

	TMap<FString, FTransform> BoneData;
	TMap<FString, float> CurveData;

	bool operator==(const FVMCData &Other) const {
		if (Port != Other.Port) return false;
		if (ServerAddress != Other.ServerAddress) return false;
		return true;
	}
};

UCLASS()
class VRM4UCAPTURE_API UVRM4U_VMCSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

	FCriticalSection cs;

public:

	UFUNCTION(BlueprintCallable, Category = VRM4U)
	bool CreateVMCServer(const FString ServerAddress, int port);

	UFUNCTION(BlueprintCallable, Category = VRM4U)
	void DestroyVMCServer(const FString ServerAddress, int port);

	static void OSCReceivedMessageEvent(const FOSCMessage& Message, const FString& IPAddress, uint16 Port);

	
	UOSCServer* FindOrAddServer(const FString ServerAddress, int port);

	TArray< FVMCData > ServerDataList_Cache;
	TArray< FVMCData > ServerDataList_Latest;
	TArray< TStrongObjectPtr<UOSCServer> > OSCServerList;

	bool CopyVMCData(FVMCData& DstData, FString serverName, int port);
	bool CopyVMCDataAll(FVMCData& DstData);

private:
	FVMCData* FindOrAddVMCData(FString serverName, int port);
};
