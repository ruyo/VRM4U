// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "Engine/DataAsset.h"
#include "VrmImportMaterialSet.generated.h"

class UMaterialInterface;

UCLASS(BlueprintType)
class VRM4U_API UVrmImportMaterialSet : public UDataAsset{

	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	UMaterialInterface* Opaque;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	UMaterialInterface* OpaqueTwoSided;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	UMaterialInterface* Translucent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
	UMaterialInterface* TranslucentTwoSided;
};

