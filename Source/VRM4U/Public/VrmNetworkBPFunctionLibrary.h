// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"


#include "VrmNetworkBPFunctionLibrary.generated.h"

UCLASS()
class VRM4U_API UVrmNetworkBPFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	static void VRMGetIPAddressTable(TArray<FString> &AddressTable);
};