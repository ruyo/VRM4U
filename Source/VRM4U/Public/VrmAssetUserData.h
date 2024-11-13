// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "Engine/AssetUserData.h"
#include "VrmAssetUserData.generated.h"

class UVrmAssetListObject;

UCLASS(BlueprintType)
class VRM4U_API UVrmAssetUserData : public UAssetUserData
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	TObjectPtr<UVrmAssetListObject> VrmAssetListObject = nullptr;

};
