// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
//#include "VrmAssetListObject.h"
#include "VrmRuntimeSettings.generated.h"

/**
 * 
 */
UCLASS(config=Engine, defaultconfig)
class VRM4U_API UVrmRuntimeSettings : public UObject
{
	
	GENERATED_UCLASS_BODY()

		// Enables experimental *incomplete and unsupported* texture atlas groups that sprites can be assigned to
		//UPROPERTY(EditAnywhere, config, Category=Settings)
		//bool bEnableSpriteAtlasGroups;

	/** For runtime load sample map */
	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (
		ConfigRestartRequired=true
		))
	uint32 bDropVRMFileEnable:1;

	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (
		ConfigRestartRequired = true
		))
	bool bAllowAllAssimpFormat = false;

	//UPROPERTY(config, EditAnywhere, Category = Settings, meta = (AllowedClasses = "VrmAssetListObject", ExactClass = false))

	/** Default material set for runtimeload */
	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (
		AllowedClasses = "/Script/CoreUObject.Object",
		ExactClass = false
		))
	FSoftObjectPath AssetListObject;

	/** priority<100 for VRM4U. Default plugins priority=100 */
	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (
		ConfigRestartRequired = true
		))
	int32 ImportPriority = 90;

	UPROPERTY(config, EditAnywhere, Category = Settings, meta = (
		ConfigRestartRequired = true
		))
	TArray<FString> extList;


#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
#endif

};
