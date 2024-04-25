// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


/*

// VRMSpringBone.cs
MIT License

Copyright (c) 2018 DWANGO Co., Ltd. for UniVRM
Copyright (c) 2018 ousttrue for UniGLTF, UniHumanoid
Copyright (c) 2018 Masataka SUMI for MToon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "AnimNode_VrmSpringBone.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SceneInterface.h"
#include "DrawDebugHelpers.h"

#include "VrmMetaObject.h"
#include "VrmUtil.h"

#include <algorithm>
/////////////////////////////////////////////////////
// FAnimNode_ModifyBone


class VRM4U_API VrmSpringBone{
public:
	VrmSpringBone();
	~VrmSpringBone();
};

namespace VRMSpringBone {
	class VRMSpringManagerBase {
	public:
		VRMSpringManagerBase() {}
		virtual ~VRMSpringManagerBase() {}
		bool bInit = false;
		USkeletalMesh* skeletalMesh = nullptr;
		const UVrmMetaObject* vrmMetaObject = nullptr;

		virtual void init(const UVrmMetaObject* meta, FComponentSpacePoseContext& Output) {}
		virtual void update(const FAnimNode_VrmSpringBone* animNode, float DeltaTime, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {}
		virtual void reset() {}
		virtual void applyToComponent(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {}

	};
}

namespace VRMSpringBone {

	class VRMSpringCollider {
	public:
		FVector offset = FVector::ZeroVector;
		float radius = 0.f;
	};
	class VRMSpringColliderGroup {
	public:
		int node = 0;
		FName node_name;

		TArray<VRMSpringCollider> colliders;
	};

	class VRMSpringData {
	public:
		int boneIndex = -1;
		FName boneName;
		FVector m_currentTail = FVector::ZeroVector;
		FVector m_prevTail = FVector::ZeroVector;
		//FTransform m_transform = FTransform::Identity;
		FVector m_boneAxis = FVector::ForwardVector;
		float m_length = 1.f;

		FQuat m_resultQuat = FQuat::Identity;
	};

	class VRMSpring {
	public:
		float stiffness = 0.f;
		float gravityPower = 0.f;
		FVector gravityDir = { 0,0,0 };
		float dragForce = 0.f;
		float hitRadius = 0.f;

		//int boneNum = 0;
		//int *bones;
		//FString bones_name;

		//int colliderGourpNum = 0;
		//int* colliderGroups = nullptr;
		TArray<int> ColliderGroupIndexArray;


		TArray< TArray<VRMSpringData> > SpringDataChain;
		TArray<VRMSpringData> RootSpringData;

		USkeletalMesh* skeletalMesh = nullptr;
		~VRMSpring() {
			skeletalMesh = nullptr;
		}

		void Update(const FAnimNode_VrmSpringBone* animNode, float DeltaTime, FTransform center,
			const TArray<VRMSpringColliderGroup>& colliderGroup,
			FComponentSpacePoseContext& Output);
	};

	class VRMSpringManager : public VRMSpringManagerBase {
	public:

		virtual void init(const UVrmMetaObject* meta, FComponentSpacePoseContext& Output) override;
		virtual void update(const FAnimNode_VrmSpringBone* animNode, float DeltaTime, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
		virtual void reset() override;

		virtual void applyToComponent(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;

		TArray<VRMSpring> spring;
		TArray<VRMSpringColliderGroup> colliderGroup;
	};

}


namespace VRM1Spring {

	class SpringBoneJointState {
	public:
		FVector prevTail;
		FVector currentTail;

		FVector initialTail;
		FVector boneAxis;
		float boneLength = 0.f;
		FTransform initialLocalMatrix;
		FQuat initialLocalRotation;

		FQuat resultQuat;
	};


	class VRM1SpringManager : public VRMSpringBone::VRMSpringManagerBase {
	public:

		TMap< int, SpringBoneJointState > JointStateMap;

		virtual void init(const UVrmMetaObject* meta, FComponentSpacePoseContext& Output) override;
		virtual void update(const FAnimNode_VrmSpringBone* animNode, float DeltaTime, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
		virtual void reset() override;
		virtual void applyToComponent(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	};
}
