// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Animation/Skeleton.h"
#include "Misc/EngineVersionComparison.h"
//#include "VrmSkeleton.generated.h"

class USkeletalMesh;
class UVrmAssetListObject;

class VRM4ULOADER_API VRMSkeleton {
public:
	static void readVrmBone(struct aiScene* scene, int& boneOffset, FReferenceSkeleton& RefSkeleton, UVrmAssetListObject* assetList);

	static void addIKBone(class UVrmAssetListObject* vrmAssetList, USkeletalMesh* sk);
};
