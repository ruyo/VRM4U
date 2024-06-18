// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

//#if WITH_EDITOR
//#include "UnrealEd.h"
//#include "AssetTypeActions_Base.h"
//#endif

#include "Vrm1LicenseObject.generated.h"

class UTexture2D;
/**
 * 
 */

USTRUCT(BlueprintType, meta = (AutoExpandCategories = "License"))
struct FLicenseStringDataPair {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	FString key;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	FString value;
};

USTRUCT(BlueprintType, meta = (AutoExpandCategories = "License"))
struct FLicenseStringDataArray {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	FString key;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	TArray<FString> value;
};

USTRUCT(BlueprintType, meta = (AutoExpandCategories = "License"))
struct FLicenseBoolDataPair {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	FString key;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	bool value = false;
};

UCLASS(Blueprintable, BlueprintType, meta = (AutoExpandCategories = "License"))
class VRM4U_API UVrm1LicenseObject : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Information)
	UTexture2D* thumbnail = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	TArray<FLicenseStringDataPair> LicenseString;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	TArray<FLicenseStringDataArray> LicenseStringArray;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "License")
	TArray<FLicenseBoolDataPair> LicenseBool;

	
};

