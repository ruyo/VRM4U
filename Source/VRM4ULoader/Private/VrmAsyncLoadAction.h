// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "Engine/LatentActionManager.h"
#include "LatentActions.h"
#include "VrmUtil.h"

class UVrmAssetListObject;
struct FImportOptionData;

class FVrmAsyncLoadActionParam {
public:
	const UVrmAssetListObject* InVrmAsset;
	UVrmAssetListObject*& OutVrmAsset;
	const FImportOptionData OptionForRuntimeLoad;
	const FString filepath;
	const uint8* pData;
	size_t dataSize;
};


class FVrmAsyncLoadAction : public FPendingLatentAction
{
public:
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	int SequenceCount = 0;
	FGraphEventRef t2 = nullptr;

	FVrmAsyncLoadActionParam param;

	FVrmAsyncLoadAction(const FLatentActionInfo& LatentInfo, FVrmAsyncLoadActionParam &);

	virtual void UpdateOperation(FLatentResponse& Response) override;

#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		//static const FNumberFormattingOptions DelayTimeFormatOptions = FNumberFormattingOptions()
		//	.SetMinimumFractionalDigits(3)
		//	.SetMaximumFractionalDigits(3);
		//return FText::Format(NSLOCTEXT("DelayAction", "DelayActionTimeFmt", "Delay ({0} seconds left)"), FText::AsNumber(TimeRemaining, &DelayTimeFormatOptions)).ToString();
		return FString("");
	}
#endif
};



