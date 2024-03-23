// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once
#include "Engine/LatentActionManager.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VrmCaptureManager.generated.h"

class UVrmMocopiReceiver;

UCLASS()
class VRM4UCAPTURE_API UVrmCaptureManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static UVrmMocopiReceiver* CreateVrmMocopiReceiver(FString ReceiveIPAddress, int32 Port, bool bMulticastLoopback, bool bStartListening, FString ServerName, UObject* Outer = nullptr);


};
