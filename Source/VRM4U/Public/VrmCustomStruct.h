// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "VrmCustomStruct.generated.h"

USTRUCT(BlueprintType)
struct FVRMRetargetSrcAnimSequence
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	class UAnimSequenceBase *AnimSequence = nullptr;
};


namespace {
	//bool operator<(const FBoneTransform &a, const FBoneTransform &b) {
	//	return a.BoneIndex < b.BoneIndex;
	//}
}
