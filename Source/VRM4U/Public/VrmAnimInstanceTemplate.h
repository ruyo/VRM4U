// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"
#include "Misc/EngineVersionComparison.h"

#include "VrmAnimInstanceTemplate.generated.h"

class UIKRetargeter;
class UVrmMetaObject;

USTRUCT()
struct VRM4U_API FVrmAnimInstanceTemplateProxy : public FAnimInstanceProxy {

public:
	GENERATED_BODY()

	FVrmAnimInstanceTemplateProxy();
	FVrmAnimInstanceTemplateProxy(UAnimInstance* InAnimInstance);
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class VRM4U_API UVrmAnimInstanceTemplate : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VRM4U)
	UIKRetargeter *VrmRetargeter = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = VRM4U)
	UVrmMetaObject* VrmMetaObject = nullptr;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	FVrmAnimInstanceTemplateProxy *myProxy = nullptr;
public:
	FVrmAnimInstanceTemplateProxy *GetProxy() { return myProxy; }
};
