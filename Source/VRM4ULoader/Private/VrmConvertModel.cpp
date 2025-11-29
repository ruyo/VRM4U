// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// The code associated with the initialization of the mesh is based on the Engine source.
// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VrmConvertModel.h"
#include "VrmConvert.h"

#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"
#include "VrmSkeleton.h"
#include "LoaderBPFunctionLibrary.h"
#include "VRM4ULoaderLog.h"

#if	UE_VERSION_OLDER_THAN(5,1,0)
#else
#include "VrmAssetUserData.h"
#endif

#include "Engine/SkeletalMesh.h"
#include "RenderingThread.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/MorphTarget.h"
#include "Rendering/SkinWeightVertexBuffer.h"
#include "CommonFrameRates.h"

#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Internationalization/Internationalization.h"

#if	UE_VERSION_OLDER_THAN(5,5,0)
#else
#include "PhysicsEngine/SkeletalBodySetup.h"
#endif


#include "Animation/AnimSequence.h"
#include "Animation/AnimBlueprint.h"
#include "Async/ParallelFor.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <assimp/vrm/vrmmeta.h>

#if WITH_EDITOR
#include "Kismet2/KismetEditorUtilities.h"
#endif


#define LOCTEXT_NAMESPACE "VRM4U"

#if	UE_VERSION_OLDER_THAN(5,2,0)

typedef uint8 VRM4U_BONE_INFLUENCE_TYPE;
static constexpr int VRM4U_MaxBoneWeight = 255;
static constexpr float VRM4U_MaxBoneWeightFloat = 255.f;

#else

#include "BoneWeights.h"

typedef uint16 VRM4U_BONE_INFLUENCE_TYPE;

static constexpr int VRM4U_MaxBoneWeight = UE::AnimationCore::MaxRawBoneWeight;
static constexpr float VRM4U_MaxBoneWeightFloat = UE::AnimationCore::MaxRawBoneWeightFloat;

#endif

static constexpr float VRM4U_InvMaxRawBoneWeightFloat = 1.0f / VRM4U_MaxBoneWeightFloat;
static constexpr float VRM4U_BoneWeightThreshold = VRM4U_InvMaxRawBoneWeightFloat;


#if WITH_EDITOR
typedef FSoftSkinVertex FSoftSkinVertexLocal;

#else
namespace {
	struct FSoftSkinVertexLocal
	{
#if	UE_VERSION_OLDER_THAN(5,0,0)
		FVector			Position;

		// Tangent, U-direction
		FVector			TangentX;
		// Binormal, V-direction
		FVector			TangentY;
		// Normal
		FVector4		TangentZ;
#else
		FVector3f			Position;
		FVector3f			TangentX;
		FVector3f			TangentY;
		FVector4f		TangentZ;
#endif

		// UVs

#if	UE_VERSION_OLDER_THAN(5,0,0)
		FVector2D		UVs[MAX_TEXCOORDS];
#else
		FVector2f		UVs[MAX_TEXCOORDS];
#endif
		// VertexColor
		FColor			Color;
		FBoneIndexType	InfluenceBones[MAX_TOTAL_INFLUENCES];
		VRM4U_BONE_INFLUENCE_TYPE	InfluenceWeights[MAX_TOTAL_INFLUENCES];

		/** If this vert is rigidly weighted to a bone, return true and the bone index. Otherwise return false. */
		//ENGINE_API bool GetRigidWeightBone(uint8& OutBoneIndex) const;

		/** Returns the maximum weight of any bone that influences this vertex. */
		//ENGINE_API uint8 GetMaximumWeight() const;

		/**
		* Serializer
		*
		* @param Ar - archive to serialize with
		* @param V - vertex to serialize
		* @return archive that was used
		*/
		//friend FArchive& operator<<(FArchive& Ar, FSoftSkinVertex& V);
	};
}
#endif

namespace {
	struct BoneMapOpt {
		int boneIndex;
		float weight;

		BoneMapOpt() : boneIndex(0), weight(0.f) {}

		bool operator<(const BoneMapOpt &b) const{
			return weight < b.weight;
		}

	};


	TArray<FString> addedList;
}

static const aiNode* GetBoneNodeFromMeshID(const int &meshID, const aiNode *node) {

	if (node == nullptr) {
		return nullptr;
	}
	for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
		if (node->mMeshes[i] == meshID) {
			return node;
		}
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i) {
		auto a = GetBoneNodeFromMeshID(meshID, node->mChildren[i]);
		if (a) {
			return a;
		}
	}
	return nullptr;
}

static const  aiNode* GetNodeFromMeshID(int meshID, const aiScene *aiData) {
	return GetBoneNodeFromMeshID(meshID, aiData->mRootNode);
}

static int GetChildBoneLocal(const USkeleton *skeleton, const int32 ParentBoneIndex, TArray<int32> & Children) {
	Children.Reset();
	auto &r = skeleton->GetReferenceSkeleton();

	const int32 NumBones = r.GetRawBoneNum();
	for (int32 ChildIndex = ParentBoneIndex + 1; ChildIndex < NumBones; ChildIndex++)
	{
		if (ParentBoneIndex == r.GetParentIndex(ChildIndex))
		{
			Children.Add(ChildIndex);
		}
	}
	return Children.Num();
}

static void FindMeshInfo(const aiScene* scene, aiNode* node, FReturnedData& result, UVrmAssetListObject *vrmAssetList)
{
	if (VRMConverter::Options::Get().IsDebugNoMesh()) {
		return;
	}

	for (uint32 MeshNo = 0; MeshNo < node->mNumMeshes; MeshNo++)
	{
		FString Fs = UTF8_TO_TCHAR(node->mName.C_Str());
		int meshidx = node->mMeshes[MeshNo];
		aiMesh *mesh = scene->mMeshes[meshidx];
		FMeshInfo_VRM4U &mi = result.meshInfo[meshidx];

		//result.meshToIndex.FindOrAdd(mesh) = mi.Vertices.Num();

		bool bSkin = true;
		if (mesh->mNumBones == 0) {
			bSkin = false;
		}

		if (0) {
			int m = mesh->mNumVertices;
			mi.Vertices.Reserve(m);
			mi.Normals.Reserve(m);

			if (mesh->HasVertexColors(0)) {
				mi.VertexColors.Reserve(m);
			}
			if (mesh->HasTangentsAndBitangents()) {
				mi.Tangents.Reserve(m);
			}
			if (mesh->HasTextureCoords(0)) {
				mi.UV0.Reserve(m);
			}
		}

		//transform.
		aiMatrix4x4 tempTrans = node->mTransformation;
		{
			auto *p = node->mParent;
			if (p == nullptr) {
				// 原点オフセットぶんは ここでは考慮しない。頂点が2重に変換されてしまうため
				tempTrans = aiMatrix4x4();
			}
			while (p) {
				if (p->mParent == nullptr) {
					// 原点オフセットぶん。たどるのはここまで。
					break;
				}
				aiMatrix4x4 aiMat = p->mTransformation;
				tempTrans *= aiMat;

				p = p->mParent;
			}
		}


		FMatrix tempMatrix;
		tempMatrix.M[0][0] = tempTrans.a1; tempMatrix.M[0][1] = tempTrans.b1; tempMatrix.M[0][2] = tempTrans.c1; tempMatrix.M[0][3] = tempTrans.d1;
		tempMatrix.M[1][0] = tempTrans.a2; tempMatrix.M[1][1] = tempTrans.b2; tempMatrix.M[1][2] = tempTrans.c2; tempMatrix.M[1][3] = tempTrans.d2;
		tempMatrix.M[2][0] = tempTrans.a3; tempMatrix.M[2][1] = tempTrans.b3; tempMatrix.M[2][2] = tempTrans.c3; tempMatrix.M[2][3] = tempTrans.d3;
		tempMatrix.M[3][0] = tempTrans.a4; tempMatrix.M[3][1] = tempTrans.b4; tempMatrix.M[3][2] = tempTrans.c4; tempMatrix.M[3][3] = tempTrans.d4;
		mi.RelativeTransform = FTransform(tempMatrix);

		auto &useFlag = mi.vertexUseFlag;
		if (VRMConverter::Options::Get().IsOptimizeVertex()) {
			// no morphtarget
			// optimize vertex

			// use flag
			useFlag.AddZeroed(mesh->mNumVertices);
			for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
				auto &face = mesh->mFaces[f];
				for (uint32_t d = 0; d < face.mNumIndices; ++d) {
					auto &ind = face.mIndices[d];
					useFlag[ind] = true;
				}
			}

			bool hasSkipVertex = false;
			// optimize table
			TArray<uint32_t> useTable;
			useTable.AddZeroed(useFlag.Num());
			{
				int c = 0;
				for (int t = 0; t < useFlag.Num(); ++t) {
					useTable[t] = t - c;
					if (useFlag[t] == false) {
						++c;
						hasSkipVertex = true;
					}
				}
			}

			if (hasSkipVertex) {
				// replace face index
				for (uint32_t f = 0; f < mesh->mNumFaces; ++f) {
					auto& face = mesh->mFaces[f];
					for (uint32_t d = 0; d < face.mNumIndices; ++d) {
						auto& ind = face.mIndices[d];
						face.mIndices[d] = useTable[face.mIndices[d]];
					}
				}

				// replace weight index
				for (uint32_t b = 0; b < mesh->mNumBones; ++b) {
					auto& bone = mesh->mBones[b];
					for (uint32_t w = 0; w < bone->mNumWeights; ++w) {
						auto& weight = bone->mWeights[w];

						uint32_t ind = weight.mVertexId;
						weight.mVertexId = useTable[ind];
						if (useFlag[ind] == 0) {
							weight.mWeight = 0.f;
						}
					}
				}
			}
		}

		if (useFlag.Num() > 0) {
			mi.vertexIndexOptTable.SetNumZeroed(useFlag.Num());
		}

		mi.useVertexCount = 0;
		//vet
		for (uint32 j = 0; j < mesh->mNumVertices; ++j)
		{
			if ((int)j < useFlag.Num()) {
				if (useFlag[j] == false) {
					continue;
				}

				mi.vertexIndexOptTable[j] = mi.useVertexCount;
				mi.useVertexCount++;
			}

			FVector vertex = FVector(
				mesh->mVertices[j].x,
				mesh->mVertices[j].y,
				mesh->mVertices[j].z);

			vertex = mi.RelativeTransform.TransformFVector4(vertex);
			mi.Vertices.Push(vertex);

			//Normal
			if (mesh->HasNormals())
			{
				FVector normal = FVector(
					mesh->mNormals[j].x,
					mesh->mNormals[j].y,
					mesh->mNormals[j].z);

				//normal = mi.RelativeTransform.TransformFVector4(normal);
				mi.Normals.Push(normal);
			} else
			{
				mi.Normals.Push(FVector(0, 1, 0));
			}

			//UV Coordinates - inconsistent coordinates
			for (int u = 0; u < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++u) {
				if (mesh->HasTextureCoords(u))
				{
					if (mi.UV0.Num() <= u) {
						mi.UV0.Push(TArray<FVector2D>());
						if (u >= 1) {
							UE_LOG(LogVRM4ULoader, Warning, TEXT("test uv2.\n"));
						}
					}
					FVector2D uv = FVector2D(mesh->mTextureCoords[u][j].x, 1.0-mesh->mTextureCoords[u][j].y);
					mi.UV0[u].Add(uv);
				}
			}

			//Tangent
			if (mesh->HasTangentsAndBitangents())
			{
				FVector v(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
				//FProcMeshTangent meshTangent = FProcMeshTangent(v.X, v.Y, v.Z);
				mi.Tangents.Push(v);
				//mi.MeshTangents.Push(meshTangent);
			}

			//Vertex color
			if (mesh->HasVertexColors(0))
			{
				FLinearColor color = FLinearColor(
					mesh->mColors[0][j].r,
					mesh->mColors[0][j].g,
					mesh->mColors[0][j].b,
					mesh->mColors[0][j].a
				);
				mi.VertexColors.Push(color);
			}
		}
	}
}


static void FindMesh(const aiScene* scene, aiNode* node, FReturnedData& retdata, UVrmAssetListObject *vrmAssetList)
{
	FindMeshInfo(scene, node, retdata, vrmAssetList);

	for (uint32 m = 0; m < node->mNumChildren; ++m)
	{
		FindMesh(scene, node->mChildren[m], retdata, vrmAssetList);
	}
}

static UPhysicsConstraintTemplate *createConstraint(USkeletalMesh *sk, UPhysicsAsset *pa, VRM::VRMSpring &spring, FName con1, FName con2){
	UPhysicsConstraintTemplate *ct = NewObject<UPhysicsConstraintTemplate>(pa, NAME_None, RF_Transactional);
	pa->ConstraintSetup.Add(ct);

	//"skirt_01_01"
	ct->Modify(false);
	{
		FString orgname = con1.ToString() + TEXT("_") + con2.ToString();
		FString cname = orgname;
		int Index = 0;
		while(pa->FindConstraintIndex(*cname) != INDEX_NONE)
		{
			cname = FString::Printf(TEXT("%s_%d"), *orgname, Index++);
		}
		ct->DefaultInstance.JointName = *cname;
	}

	ct->DefaultInstance.ConstraintBone1 = con1;
	ct->DefaultInstance.ConstraintBone2 = con2;

	ct->DefaultInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Limited, 10);
	ct->DefaultInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 10);
	ct->DefaultInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 10);

	ct->DefaultInstance.ProfileInstance.ConeLimit.Stiffness = 100.f * spring.stiffness;
	ct->DefaultInstance.ProfileInstance.TwistLimit.Stiffness = 100.f * spring.stiffness;

	const int32 BoneIndex1 = VRMGetRefSkeleton(sk).FindBoneIndex(ct->DefaultInstance.ConstraintBone1);
	const int32 BoneIndex2 = VRMGetRefSkeleton(sk).FindBoneIndex(ct->DefaultInstance.ConstraintBone2);

	if (BoneIndex1 == INDEX_NONE || BoneIndex2 == INDEX_NONE) {
#if WITH_EDITOR
		if (VRMConverter::IsImportMode()) {
			ct->PostEditChange();
		}
#endif
		return ct;
	}

	check(BoneIndex1 != INDEX_NONE);
	check(BoneIndex2 != INDEX_NONE);

	const TArray<FTransform> &t = VRMGetRefSkeleton(sk).GetRawRefBonePose();

	const FTransform BoneTransform1 = t[BoneIndex1];//sk->GetBoneTransform(BoneIndex1);
	FTransform BoneTransform2 = t[BoneIndex2];//EditorSkelComp->GetBoneTransform(BoneIndex2);

	{
		int32 tb = BoneIndex2;
		while (1) {
			int32 p = VRMGetRefSkeleton(sk).GetRawParentIndex(tb);
			if (p < 0) break;

			const FTransform &f = t[p];
			//BoneTransform2 = f.GetRelativeTransform(BoneTransform2);

			if (p == BoneIndex1) {
				break;
			}
			tb = p;
		}
	}

	//auto b = BoneTransform1;
	ct->DefaultInstance.Pos1 = FVector::ZeroVector;
	ct->DefaultInstance.PriAxis1 = FVector(1, 0, 0);
	ct->DefaultInstance.SecAxis1 = FVector(0, 1, 0);


	auto r = BoneTransform2;// .GetRelativeTransform(BoneTransform2);
							//	auto r = BoneTransform1.GetRelativeTransform(BoneTransform2);
	auto twis = r.GetLocation().GetSafeNormal();
	auto p1 = twis;
	p1.X = p1.Z = 0.f;
	auto p2 = FVector::CrossProduct(twis, p1).GetSafeNormal();
	p1 = FVector::CrossProduct(p2, twis).GetSafeNormal();

	ct->DefaultInstance.Pos2 = -r.GetLocation();
	//ct->DefaultInstance.PriAxis2 = p1;
	//ct->DefaultInstance.SecAxis2 = p2;
	ct->DefaultInstance.PriAxis2 = r.GetUnitAxis(EAxis::X);
	ct->DefaultInstance.SecAxis2 = r.GetUnitAxis(EAxis::Y);

	// child 
	//ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame1, FTransform::Identity);
	// parent
	//ct->DefaultInstance.SetRefFrame(EConstraintFrame::Frame2, BoneTransform1.GetRelativeTransform(BoneTransform2));

#if WITH_EDITOR
	ct->SetDefaultProfile(ct->DefaultInstance);
	if (VRMConverter::IsImportMode()) {
		ct->PostEditChange();
	}
#endif
	//ct->DefaultInstance.InitConstraint();

	return ct;
}

static void CreateSwingTail(UVrmAssetListObject *vrmAssetList, VRM::VRMSpring &spring, FName &boneName, USkeletalBodySetup *bs, int BodyIndex1,
	TArray<int> &swingBoneIndexArray, int sboneIndex = -1) {

	USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
	USkeleton *k = VRMGetSkeleton(sk);
	UPhysicsAsset *pa = VRMGetPhysicsAsset(sk);

	TArray<int32> child;
	int32 ii = k->GetReferenceSkeleton().FindBoneIndex(boneName);
	GetChildBoneLocal(k, ii, child);
	for (auto &c : child) {

		if (sboneIndex >= 0) {
			c = sboneIndex;
		}

		if (addedList.Find(k->GetReferenceSkeleton().GetBoneName(c).ToString().ToLower()) >= 0) {
			continue;
		}
		addedList.Add(k->GetReferenceSkeleton().GetBoneName(c).ToString().ToLower());


		USkeletalBodySetup *bs2 = Cast<USkeletalBodySetup>(StaticDuplicateObject(bs, pa, NAME_None));

		bs2->BoneName = k->GetReferenceSkeleton().GetBoneName(c);
		bs2->PhysicsType = PhysType_Simulated;
		bs2->CollisionReponse = EBodyCollisionResponse::BodyCollision_Enabled;
		//bs2->profile

		int BodyIndex2 = pa->SkeletalBodySetups.Add(bs2);
		auto *ct = createConstraint(sk, pa, spring, boneName, bs2->BoneName);
		pa->DisableCollision(BodyIndex1, BodyIndex2);

		swingBoneIndexArray.AddUnique(BodyIndex2);

		//CreateSwingTail(vrmAssetList, spring, bs2->BoneName, bs2, BodyIndex2, swingBoneIndexArray);
		if (sboneIndex >= 0) {
			break;
		}
	}
}

static void CreateSwingHead(UVrmAssetListObject *vrmAssetList, VRM::VRMSpring &spring, FName &boneName, TArray<int> &swingBoneIndexArray, int sboneIndex) {

	USkeletalMesh *sk = vrmAssetList->SkeletalMesh;
	USkeleton *k = VRMGetSkeleton(sk);
	UPhysicsAsset *pa = VRMGetPhysicsAsset(sk);

	{
		int i = VRMGetRefSkeleton(sk).FindRawBoneIndex(boneName);
		if (i == INDEX_NONE) {
			return;
		}
	}
	//sk->RefSkeleton->GetParentIndex();

	USkeletalBodySetup *bs = nullptr;
	int BodyIndex1 = -1;

	if (addedList.Find(boneName.ToString().ToLower()) < 0) {
		addedList.Add(boneName.ToString().ToLower());

		bs = NewObject<USkeletalBodySetup>(pa, NAME_None, RF_Transactional);

		//int nodeID = aiData->mRootNode->FindNode(sbone.c_str());
		//sk->RefSkeleton.GetRawRefBoneInfo[0].
		//sk->bonetree
		//bs->constrai
		FKAggregateGeom agg;
		FKSphereElem SphereElem;
		SphereElem.Center = FVector(0);
		SphereElem.Radius = spring.hitRadius * 100.f;
		agg.SphereElems.Add(SphereElem);


		bs->Modify();
		bs->BoneName = boneName;
		bs->AddCollisionFrom(agg);
		bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		// newly created bodies default to simulating
		bs->PhysicsType = PhysType_Kinematic;	// fix
												//bs->get
		bs->CollisionReponse = EBodyCollisionResponse::BodyCollision_Disabled;
		bs->DefaultInstance.InertiaTensorScale.Set(2, 2, 2);
		bs->DefaultInstance.LinearDamping = 10.0f * spring.dragForce;
		bs->DefaultInstance.AngularDamping = 10.0f * spring.dragForce;

		bs->InvalidatePhysicsData();
		bs->CreatePhysicsMeshes();
		BodyIndex1 = pa->SkeletalBodySetups.Add(bs);

		//pa->UpdateBodySetupIndexMap();
#if WITH_EDITOR
	//pa->InvalidateAllPhysicsMeshes();
#endif
	} else {
		for (int i = 0; i < pa->SkeletalBodySetups.Num(); ++i) {
			auto &a = pa->SkeletalBodySetups[i];
			if (a->BoneName != boneName) {
				continue;
			}

			BodyIndex1 = i;
			bs = a;
			break;
		}
	}

	if (BodyIndex1 >= 0) {
		CreateSwingTail(vrmAssetList, spring, boneName, bs, BodyIndex1, swingBoneIndexArray);
	}
}

bool VRMConverter::ConvertModel(UVrmAssetListObject *vrmAssetList) {
	if (vrmAssetList == nullptr) {
		return false;
	}

	vrmAssetList->MeshReturnedData = MakeShareable(new FReturnedData());
	FReturnedData &result = *(vrmAssetList->MeshReturnedData);
	//FReturnedData &result = *(vrmAssetList->Result);

	result.bSuccess = false;
	result.meshInfo.Empty();
	result.NumMeshes = 0;

	if (aiData == nullptr)
	{
		UE_LOG(LogVRM4ULoader, Warning, TEXT("test null.\n"));
	}

	if (aiData->HasMeshes() && VRMConverter::Options::Get().IsDebugNoMesh() == false)
	{

		// !! before remove unused vertex !!
		// remove degenerate triangles
		//
		if (Options::Get().IsRemoveDegenerateTriangles()) {
			for (uint32 meshNo = 0; meshNo < aiData->mNumMeshes; ++meshNo)
			{
				//Triangle number
				for (uint32 faceNo = 0; faceNo < aiData->mMeshes[meshNo]->mNumFaces; ++faceNo)
				{
					for (uint32 indexNo = 0; indexNo < aiData->mMeshes[meshNo]->mFaces[faceNo].mNumIndices; ++indexNo)
					{
						if (indexNo >= 3) {
							//UE_LOG(LogVRM4ULoader, Warning, TEXT("FindMeshInfo. %d\n"), m);
						}

						if ((indexNo % 3 == 0) && (indexNo + 2 < aiData->mMeshes[meshNo]->mFaces[faceNo].mNumIndices)) {
							const unsigned int tmpIndex[] = {
								aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo],
								aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo + 1],
								aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo + 2],
							};
							aiVector3D v[] = {
								aiData->mMeshes[meshNo]->mVertices[tmpIndex[0]],
								aiData->mMeshes[meshNo]->mVertices[tmpIndex[1]],
								aiData->mMeshes[meshNo]->mVertices[tmpIndex[2]],
							};
							v[0] -= v[2];
							v[1] -= v[2];

							bool bDel = false;

							if ((v[0] ^ v[1]).SquareLength() == 0) {
								UE_LOG(LogVRM4ULoader, Warning, TEXT("degenerate face %d"), faceNo);

								// del
								aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo]   = 0;
								aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo+1] = 0;
								aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo+2] = 0;
								indexNo += 2;
								continue;
							}
						}
					}
				}
			}
		}
		result.meshInfo.SetNum(aiData->mNumMeshes);


		// find and remove unused vertex
		FindMesh(aiData, aiData->mRootNode, result, vrmAssetList);

		for (uint32 meshNo = 0; meshNo < aiData->mNumMeshes; ++meshNo)
		{
			//Triangle number
			for (uint32 faceNo = 0; faceNo < aiData->mMeshes[meshNo]->mNumFaces; ++faceNo)
			{
				for (uint32 indexNo = 0; indexNo < aiData->mMeshes[meshNo]->mFaces[faceNo].mNumIndices; ++indexNo)
				{
					if (indexNo >= 3) {
						//UE_LOG(LogVRM4ULoader, Warning, TEXT("FindMeshInfo. %d\n"), m);
					}
					if ((indexNo % 3 == 0) && (indexNo + 2 < aiData->mMeshes[meshNo]->mFaces[faceNo].mNumIndices)) {
						int tmp = aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo]
							+ aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo + 1]
							+ aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo + 2];
						if (tmp == 0) {
							// remove
							indexNo += 2;
							continue;
						}
					}

					result.meshInfo[meshNo].Triangles.Push(aiData->mMeshes[meshNo]->mFaces[faceNo].mIndices[indexNo]);

				}
			}
		}
		result.bSuccess = true;
	}

	bool bReimportMode = false;

	USkeletalMesh *sk = nullptr;
	if (vrmAssetList->Package == GetTransientPackage()) {
		sk = VRM4U_NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
	}else {
		TArray<UObject*> ret;
		FString name = (FString(TEXT("SK_")) + vrmAssetList->BaseFileName);
		bool bExistAsset = false;
		GetObjectsWithOuter(vrmAssetList->Package, ret);
		for (auto *a : ret) {
			auto s = a->GetName().ToLower();
			if (s.IsEmpty()) continue;

			if (s == name.ToLower()) {
				static int ccc = 0;
				++ccc;

				if (Options::Get().IsForceOverride()) {
					a->Rename(*(FString(TEXT("need_reload_sk_VRM"))+FString::FromInt(ccc)), GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
				} else {
					bReimportMode = true;
					// reimport
					bExistAsset = true;
					sk = Cast<USkeletalMesh>(a);
					sk->MarkPackageDirty();
				}

				break;
			}
		}
		if (bExistAsset == false) {
			sk = VRM4U_NewObject<USkeletalMesh>(vrmAssetList->Package, *name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		}
	}
#if	UE_VERSION_OLDER_THAN(5,1,0)
#else
	if (sk) {
		UVrmAssetUserData*d  = NewObject<UVrmAssetUserData>(sk, NAME_None, RF_Public | RF_Transactional);
		d->VrmAssetListObject = vrmAssetList;
		sk->AddAssetUserData(d);
	}
#endif

	{
#if WITH_EDITOR
		sk->PreEditChange(NULL);
		//Dirty the DDC Key for any imported Skeletal Mesh
#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
		sk->InvalidateDeriveDataCacheGUID();
#endif
		if (bReimportMode == false) {
			FSkeletalMeshModel* ImportedResource = sk->GetImportedModel();
			ImportedResource->LODModels.Empty();
		}

#endif
		if (bReimportMode == false) {
			sk->ReleaseCPUResources();
			sk->ReleaseResources();
#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
			sk->ReleaseSkinWeightProfileResources();
#endif

#if	UE_VERSION_OLDER_THAN(4,27,0)
			sk->PhysicsAsset = nullptr;
			sk->BodySetup = nullptr;
#else
			sk->SetPhysicsAsset(nullptr);
			sk->SetBodySetup(nullptr);
#endif
		}
	}


	bool bCreateSkeleton = false;
	USkeleton *k = Options::Get().GetSkeleton();
	if (k == nullptr){
		bCreateSkeleton = true;
		if (vrmAssetList->Package == GetTransientPackage()) {
			k = VRM4U_NewObject<USkeleton>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
		}else {
			bool bExistAsset = false;
			TArray<UObject*> ret;
			FString name = (TEXT("SKEL_") + vrmAssetList->BaseFileName);
			GetObjectsWithOuter(vrmAssetList->Package, ret);

			if (VRMConverter::Options::Get().IsSingleUAssetFile() == false) {
				FString s = VRM4U_GetPackagePath(vrmAssetList->Package) / name + TEXT(".")+ name;
				auto *a = FindObject<USkeleton>(NULL, *s);
				if (a != nullptr) {
					ret.Add(a);
				}
			}

			for (auto *a : ret) {
				auto s = a->GetName().ToLower();
				if (s.IsEmpty()) continue;

				if (s == name.ToLower()) {
					if (Options::Get().IsForceOverride()) {
						a->ClearFlags(EObjectFlags::RF_Standalone);
						a->SetFlags(EObjectFlags::RF_Public | RF_Transient);
						a->Rename(*(FString(TEXT("need_reload_k_VRM")) + FString::FromInt(0)), GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
						a->ConditionalBeginDestroy();
					} else {
						// reimport
						k = Cast<USkeleton>(a);
						bExistAsset = true;
					}
					break;
				}
			}
			if (bExistAsset == false) {
				k = VRM4U_NewObject<USkeleton>(vrmAssetList->Package, *name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			}
		}
	}

	int allIndex = 0;
	int allVertex = 0;
	int uvNum = 1;
	{
		for (int meshID = 0; meshID < result.meshInfo.Num(); ++meshID) {
			allIndex += result.meshInfo[meshID].Triangles.Num();
			allVertex += result.meshInfo[meshID].Vertices.Num();
		}
		for (int meshID = 0; meshID < result.meshInfo.Num(); ++meshID) {
			auto &mInfo = result.meshInfo[meshID];
			uvNum = FMath::Max(uvNum, mInfo.UV0.Num());
			uvNum = FMath::Min(uvNum, (int)MAX_TEXCOORDS);
			if (uvNum >= 2) {
				UE_LOG(LogVRM4ULoader, Warning, TEXT("test uv2.\n"));
			}
		}
	}

	static int boneOffset = 0;
	{
		// name dup check
		VRMSetSkeleton(sk, k);
		//k->MergeAllBonesToBoneTree(src);
		{
			USkeletalMesh* sk_tmp = VRM4U_NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);
			//USkeleton *k_tmp = VRM4U_NewObject<USkeleton>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient);

			VRMSkeleton::readVrmBone(const_cast<aiScene*>(aiData), boneOffset, VRMGetRefSkeleton(sk_tmp), vrmAssetList);


			// force set PMX bonetable for addIK
			if (VRMConverter::Options::Get().IsPMXModel()) {
				// add meta pmx bone map
				for (const auto& t : VRMUtil::table_ue4_pmx) {
					FString pmxBone;
					VRMUtil::GetReplacedPMXBone(pmxBone, t.BoneVRM);

					FString targetList[2] = {
						pmxBone,
						t.BoneVRM,
					};

					bool finish = false;
					for (int i = 0; i < 2; ++i) {
						FString target = targetList[i];// t.BoneVRM;
						const FString& ue4 = t.BoneUE4;
						auto ind = VRMGetRefSkeleton(sk_tmp).FindBoneIndex(*target);
						if (ind != INDEX_NONE) {

							for (const auto& v : VRMUtil::table_ue4_vrm) {
								if (v.BoneUE4 == t.BoneUE4) {
									if (v.BoneVRM.IsEmpty()) {
										continue;
									}
									// renew bonemap
									vrmAssetList->VrmMetaObject->humanoidBoneTable.Add(v.BoneVRM) = target;
									finish = true;
									break;
								}
							}
						}
						if (finish) break;
					}// 2 loop
					vrmAssetList->VrmMetaObject->humanoidBoneTable.Add("leftEye") = TEXT("左目");
					vrmAssetList->VrmMetaObject->humanoidBoneTable.Add("rightEye") = TEXT("右目");
				}
			}// pmx bonetable end


			VRMSkeleton::addIKBone(vrmAssetList, sk_tmp);

			//k_tmp->readVrmBone(const_cast<aiScene*>(aiData), boneOffset);
			//k_tmp->addIKBone(vrmAssetList);
			//sk_tmp->Skeleton = k_tmp;
			//sk_tmp->RefSkeleton = k_tmp->GetReferenceSkeleton();

			//k_tmp->SetPreviewMesh(sk_tmp);
			//k_tmp->UpdateReferencePoseFromMesh(sk_tmp);
			//k_tmp->RecreateBoneTree(sk_tmp);

			bool b = k->MergeAllBonesToBoneTree(sk_tmp);
			if (b == false) {
				// import failure
				UE_LOG(LogVRM4ULoader, Error, TEXT("VRM: Failed MergeAllBonesToBoneTree \"%s\" to \"%s\""), *sk->GetName(), *k->GetName());

				VRMSetSkeleton(sk, nullptr);

				sk->ClearFlags(EObjectFlags::RF_Standalone);
				sk->SetFlags(EObjectFlags::RF_Public | RF_Transient);
				sk->ConditionalBeginDestroy();
				if (bCreateSkeleton) {
					k->ClearFlags(EObjectFlags::RF_Standalone);
					k->SetFlags(EObjectFlags::RF_Public | RF_Transient);
					k->ConditionalBeginDestroy();
				}

				return false;
			}

			VRMSetRefSkeleton(sk, k->GetReferenceSkeleton());

			if (Options::Get().GetSkeleton()) {
				// update refpose
				FReferenceSkeletonModifier RefSkelModifier(VRMGetRefSkeleton(sk), k);
				{
					FTransform t;
					t.SetIdentity();
					const auto& srcRef = VRMGetRefSkeleton(sk_tmp);
					auto& dstRef = VRMGetRefSkeleton(sk);

					for (int c = 0; c < srcRef.GetRawBoneNum(); ++c) {
						auto name = srcRef.GetBoneName(c);
						auto ind = dstRef.FindRawBoneIndex(name);

						if (ind >= 0) {
							auto pose = srcRef.GetRawRefBonePose()[c];

							RefSkelModifier.UpdateRefPoseTransform(ind, pose);
						}
					}

					//for (int i = 0; i < sk->RefSkeleton.GetRawBoneNum(); ++i) {
					//	RefSkelModifier.UpdateRefPoseTransform(i, t);
					//}
				}
			}
		}

		sk->CalculateInvRefMatrices();
		sk->CalculateExtendedBounds();
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(4,20,0)
#else
		sk->UpdateGenerateUpToData();
#endif
		sk->ConvertLegacyLODScreenSize();
		sk->GetImportedModel()->LODModels.Reset();
#endif

#if WITH_EDITORONLY_DATA
		k->SetPreviewMesh(sk);
#endif
		if (Options::Get().GetSkeleton() == nullptr) {
			k->RecreateBoneTree(sk);
		}

#if	UE_VERSION_OLDER_THAN(5,0,0)
		{
			TArray<FName> BonesToRemove;
			k->RemoveVirtualBones(BonesToRemove);
		}
#endif

		// sk end

		vrmAssetList->SkeletalMesh = sk;

		if (sk->GetLODInfo(0) == nullptr) {
#if	UE_VERSION_OLDER_THAN(4,20,0)
			sk->LODInfo.AddZeroed(1);
			//const USkeletalMeshLODSettings* DefaultSetting = sk->GetDefaultLODSetting();
			// if failed to get setting, that means, we don't have proper setting 
			// in that case, use last index setting
			//!DefaultSetting->SetLODSettingsToMesh(sk, 0);
#else
			FSkeletalMeshLODInfo& info = sk->AddLODInfo();
#endif
		}


		if (VRMConverter::Options::Get().IsVRM10Model() && VRMConverter::Options::Get().IsVRM10Bindpose() == false
			&& VRMConverter::Options::Get().IsDebugOneBone() == false
			&& VRMConverter::Options::Get().IsVRM10BindToRestPose() == true) {
			if (vrmAssetList->Pose_bind.Num() == 0 || vrmAssetList->Pose_tpose.Num() == 0) {
				UE_LOG(LogVRM4ULoader, Warning, TEXT("BindPose -> TPose :: no bindpose array!"));
			}
			else {
				auto& info = vrmAssetList->MeshReturnedData->meshInfo;
				struct WeightData {
					FString boneName;
					float weight = 0;
				};
				TMap<int, TArray<WeightData> > weightTable;
				auto* scene = const_cast<aiScene*>(aiData);

				// generate weightTable
				{
					int vertexOffset = 0;
					for (uint32_t meshNo = 0; meshNo < scene->mNumMeshes; ++meshNo) {
						auto* mesh = scene->mMeshes[meshNo];
						for (uint32_t boneNo = 0; boneNo < mesh->mNumBones; ++boneNo) {
							auto* bone = mesh->mBones[boneNo];
							for (uint32_t weightNo = 0; weightNo < bone->mNumWeights; ++weightNo) {
								auto weight = bone->mWeights[weightNo];

								WeightData d;
								d.boneName = UTF8_TO_TCHAR(bone->mName.C_Str());
								d.weight = weight.mWeight;
								weightTable.FindOrAdd(vertexOffset + weight.mVertexId).Add(d);
							}
						}
						vertexOffset += mesh->mNumVertices;
					}
					// weight check
					for (auto w : weightTable) {
						float f = 0.f;
						for (auto data : w.Value) {
							f += data.weight;
						}
						if (fabs(f - 1.f) > 0.01f) {
							UE_LOG(LogVRM4ULoader, Warning, TEXT("BindPose -> TPose :: bad weight!"));
						}
					}
				}// end weightTable

				// bind pose -> t pose
				{
					int vertexOffset = 0;
					for (uint32_t meshNo = 0; meshNo < scene->mNumMeshes; ++meshNo) {
						if (info.IsValidIndex(meshNo) == false) {
							break;
						}
						auto* mesh = scene->mMeshes[meshNo];

						for (int vertexNo = 0; vertexNo < info[meshNo].Vertices.Num(); ++vertexNo) {
							FVector v_orig = info[meshNo].Vertices[vertexNo];
							//v_orig.Set(mesh->mVertices[vertexNo].x, mesh->mVertices[vertexNo].y, mesh->mVertices[vertexNo].z);
							v_orig.Set(mesh->mVertices[vertexNo].x, -mesh->mVertices[vertexNo].z, mesh->mVertices[vertexNo].y);
							FVector v(0, 0, 0);

							if (weightTable.Find(vertexOffset + vertexNo) == nullptr) {
								UE_LOG(LogVRM4ULoader, Warning, TEXT("BindPose -> TPose :: no weight data %d"), vertexOffset + vertexNo);
								continue;
							}
							for (auto a : weightTable[vertexOffset + vertexNo]) {
								auto tpose = vrmAssetList->Pose_tpose.Find(a.boneName);
								auto bpose = vrmAssetList->Pose_bind.Find(a.boneName);

								if (tpose && bpose) {
									FVector v_diff = (bpose->Inverse() * *tpose).TransformPosition(v_orig * 100.f);
									v += v_diff * a.weight;
#if	UE_VERSION_OLDER_THAN(5,1,0)
									float len = v.Size();
#else
									float len = v.Length();
#endif
									if (len >= 100000) {
										UE_LOG(LogVRM4ULoader, Warning, TEXT("BindPose -> TPose :: bad weight!"));
									}
								}
								else {
									UE_LOG(LogVRM4ULoader, Warning, TEXT("BindPose -> TPose :: no pose transform %p %p"), tpose, bpose);
								}
							}
							v.Set(v.X, v.Z, -v.Y);
							info[meshNo].Vertices[vertexNo] = v / 100.f;
						}
						vertexOffset += mesh->mNumVertices;
					}
				}
			}
		}// end bind -> t pose

		// begin vertex


		//sk->CacheDerivedData();
		if (bReimportMode == false) {
			sk->AllocateResourceForRendering();
		}
		FSkeletalMeshRenderData* p = sk->GetResourceForRendering();
		//p->Cache(sk);
		//sk->OnPostMeshCached().Broadcast(sk);

		if (p->LODRenderData.Num() == 0) {
#if	UE_VERSION_OLDER_THAN(4,23,0)
			new(p->LODRenderData) FSkeletalMeshLODRenderData();
#else
			auto* tmp = new FSkeletalMeshLODRenderData();
			p->LODRenderData.Add(tmp);
#endif
		}
		FSkeletalMeshLODRenderData* pRd = &p->LODRenderData[0];

		{
			if (allVertex > 0) {
				pRd->StaticVertexBuffers.PositionVertexBuffer.Init(allVertex);
				pRd->StaticVertexBuffers.ColorVertexBuffer.InitFromSingleColor(FColor(255, 255, 255, 255), allVertex);
				pRd->StaticVertexBuffers.StaticMeshVertexBuffer.CleanUp();
				pRd->StaticVertexBuffers.StaticMeshVertexBuffer.Init(allVertex, uvNum);
			}


#if WITH_EDITOR
			TArray<FSoftSkinVertex> Weight;
			Weight.SetNum(allVertex);
			pRd->SkinWeightVertexBuffer.Init(Weight);
#else

			{
#if	UE_VERSION_OLDER_THAN(4,25,0)
				TArray< TSkinWeightInfo<false> > InWeights;
#else
				TArray<FSkinWeightInfo> InWeights;
#endif

				InWeights.SetNum(allVertex);
				for (auto& a : InWeights) {
					memset(a.InfluenceBones, 0, sizeof(a.InfluenceBones));
					memset(a.InfluenceWeights, 0, sizeof(a.InfluenceWeights));
				}
				pRd->SkinWeightVertexBuffer = InWeights;
			}
#endif
		}

		sk->InitResources();


		if (VRMConverter::Options::Get().IsDebugNoMaterial()) {
			//VRMGetMaterials(sk).SetNum(vrmAssetList->Materials.Num());
		}else{
			VRMGetMaterials(sk).SetNum(vrmAssetList->Materials.Num());
		}
		for (int i = 0; i < VRMGetMaterials(sk).Num(); ++i) {
			VRMGetMaterials(sk)[i].MaterialInterface = vrmAssetList->Materials[i];
			VRMGetMaterials(sk)[i].MaterialSlotName = UTF8_TO_TCHAR(aiData->mMaterials[i]->GetName().C_Str());

			VRMGetMaterials(sk)[i].UVChannelData = FMeshUVChannelInfo(1);
		}
		if (VRMGetMaterials(sk).Num() == 0) {
			VRMGetMaterials(sk).SetNum(1);
		}
	}
	{
		FSkeletalMeshLODRenderData &rd = sk->GetResourceForRendering()->LODRenderData[0];

		FVector BoundMin(-100, -100, 0);
		FVector BoundMax(100, 100, 200);

		TArray<int> AllActiveBones;

		{
			int boneNum = VRMGetSkeleton(sk)->GetReferenceSkeleton().GetRawBoneNum();
			rd.RequiredBones.SetNum(boneNum);
			rd.ActiveBoneIndices.SetNum(boneNum);
			for (int i = 0; i < boneNum; ++i) {
				rd.RequiredBones[i] = i;
				rd.ActiveBoneIndices[i] = i;
			}
		}
		{
			FStaticMeshVertexBuffers	 &v = rd.StaticVertexBuffers;

			FSoftSkinVertexLocal softSkinVertexLocalZero;
			{
#if	UE_VERSION_OLDER_THAN(4,20,0)
				{
					FPackedNormal n(0);
					softSkinVertexLocalZero.Position = FVector::ZeroVector;
					softSkinVertexLocalZero.TangentX = softSkinVertexLocalZero.TangentY = softSkinVertexLocalZero.TangentZ = n;
				}
#elif	UE_VERSION_OLDER_THAN(5,0,0)
				softSkinVertexLocalZero.Position = softSkinVertexLocalZero.TangentX = softSkinVertexLocalZero.TangentY = FVector::ZeroVector;
				softSkinVertexLocalZero.TangentZ.Set(0, 0, 0, 1);
#else
				softSkinVertexLocalZero.Position = softSkinVertexLocalZero.TangentX = softSkinVertexLocalZero.TangentY = FVector3f::Zero();
				softSkinVertexLocalZero.TangentZ.Set(0, 0, 0, 1);
#endif
				softSkinVertexLocalZero.Color = FColor::White;

				memset(softSkinVertexLocalZero.UVs, 0, sizeof(softSkinVertexLocalZero.UVs));
				memset(softSkinVertexLocalZero.InfluenceBones, 0, sizeof(softSkinVertexLocalZero.InfluenceBones));
				memset(softSkinVertexLocalZero.InfluenceWeights, 0, sizeof(softSkinVertexLocalZero.InfluenceWeights));
			}

			TArray<uint32> Triangles;
			TArray<FSoftSkinVertexLocal> Weight;
			Weight.SetNum(allVertex);
			for (auto &w : Weight) {
				w = softSkinVertexLocalZero;
			}

			//Weight.AddZeroed(allVertex);
			int currentIndex = 0;
			int currentVertex = 0;

#if WITH_EDITORONLY_DATA
			if (sk->GetImportedModel()->LODModels.Num() == 0) {
#if	UE_VERSION_OLDER_THAN(4,23,0)
				new(sk->GetImportedModel()->LODModels) FSkeletalMeshLODModel();
#else
				sk->GetImportedModel()->LODModels.Add(new FSkeletalMeshLODModel());
#endif
			}
			sk->GetImportedModel()->LODModels[0].Sections.Empty();
			sk->GetImportedModel()->LODModels[0].Sections.SetNum(result.meshInfo.Num());
#endif

			if (VRMConverter::IsImportMode() == false) {
				rd.RenderSections.Empty();
				rd.RenderSections.SetNum(result.meshInfo.Num());
			}
			for (int meshID = 0; meshID < result.meshInfo.Num(); ++meshID) {
				TArray<FSoftSkinVertexLocal> meshWeight;
				auto &mInfo = result.meshInfo[meshID];

				for (int i = 0; i < mInfo.Vertices.Num(); ++i) {
					FSoftSkinVertexLocal *meshS = new(meshWeight) FSoftSkinVertexLocal();
					*meshS = softSkinVertexLocalZero;
					auto a = result.meshInfo[meshID].Vertices[i] * 100.f;

					v.PositionVertexBuffer.VertexPosition(currentVertex + i).Set(-a.X, a.Z, a.Y);
					if (VRMConverter::Options::Get().IsVRM10Model()) {
						v.PositionVertexBuffer.VertexPosition(currentVertex + i).Set(a.X, -a.Z, a.Y);
					}else if (VRMConverter::Options::Get().IsPMXModel() || VRMConverter::Options::Get().IsBVHModel()) {
						v.PositionVertexBuffer.VertexPosition(currentVertex + i).X *= -1.f;
						v.PositionVertexBuffer.VertexPosition(currentVertex + i).Y *= -1.f;
					}
					v.PositionVertexBuffer.VertexPosition(currentVertex + i) *= VRMConverter::Options::Get().GetModelScale();

					for (int u=0; u < FMath::Min(mInfo.UV0.Num(), (int)MAX_TEXCOORDS); ++u){
						FVector2D uv(0, 0);
						if (i < mInfo.UV0[u].Num()) {
							uv = mInfo.UV0[u][i];
						}
#if	UE_VERSION_OLDER_THAN(5,0,0)
						v.StaticMeshVertexBuffer.SetVertexUV(currentVertex + i, u, uv);
						meshS->UVs[u] = uv;
#else
						v.StaticMeshVertexBuffer.SetVertexUV(currentVertex + i, u, FVector2f(uv));
						meshS->UVs[u] = FVector2f(uv);
#endif
					}

					if (i < mInfo.Tangents.Num()){
						//v.StaticMeshVertexBuffer.SetVertexTangents(currentVertex + i, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
						//v.StaticMeshVertexBuffer.SetVertexTangents(currentVertex + i, result.meshInfo[meshID].Tangents);
						auto &n = mInfo.Normals[i];
						FVector n_tmp(-n.X, n.Z, n.Y);
						FVector t_tmp(-mInfo.Tangents[i].X, mInfo.Tangents[i].Z, mInfo.Tangents[i].Y);

						if (VRMConverter::Options::Get().IsVRM10Model()) {
							n_tmp.Set(n.X, -n.Z, n.Y);
							t_tmp.Set(mInfo.Tangents[i].X, -mInfo.Tangents[i].Z, mInfo.Tangents[i].Y);
						}
						if (VRMConverter::Options::Get().IsPMXModel() || VRMConverter::Options::Get().IsBVHModel()) {
							FVector tmpv(-1, -1, 1);
							n_tmp *= tmpv;
							t_tmp *= tmpv;
						}

						t_tmp.Normalize();
						n_tmp.Normalize();

#if	UE_VERSION_OLDER_THAN(5,0,0)
						meshS->TangentX = t_tmp;
						meshS->TangentY = n_tmp ^ t_tmp;
						meshS->TangentZ = n_tmp;
#else
						meshS->TangentX = FVector3f(t_tmp);
						meshS->TangentY = FVector3f(n_tmp ^ t_tmp);
						meshS->TangentZ = FVector4f(FVector3f(n_tmp), 1);
#endif

						v.StaticMeshVertexBuffer.SetVertexTangents(currentVertex + i, meshS->TangentX, meshS->TangentY, meshS->TangentZ);

					}

					if (i < mInfo.VertexColors.Num()) {
						auto &c = mInfo.VertexColors[i];
						meshS->Color = FColor(c.R, c.G, c.B, c.A);
					}

					FSoftSkinVertexLocal &s = Weight[currentVertex + i];
					s.Position = v.PositionVertexBuffer.VertexPosition(currentVertex + i);
					meshS->Position = v.PositionVertexBuffer.VertexPosition(currentVertex + i);

				} // vertex loop

				auto &aiM = aiData->mMeshes[meshID];
				TArray<int> bonemap;
				TArray<BoneMapOpt> boneAll;
				/* skip! no first bone
				{
					bonemap.Add(0);
					BoneMapOpt o;
					o.boneIndex = 0;
					o.weight = -1.f;
					boneAll.Add(o);
				}
				*/
				//aiData->mRootNode->mMeshes
				for (uint32 boneIndex = 0; boneIndex < aiM->mNumBones; ++boneIndex) {
					auto& aiB = aiM->mBones[boneIndex];

					const int b = VRMGetRefSkeleton(sk).FindBoneIndex(UTF8_TO_TCHAR(aiB->mName.C_Str()));

					if (b < 0) {
						continue;
					}
					for (uint32 weightIndex = 0; weightIndex < aiB->mNumWeights; ++weightIndex) {
						auto& aiW = aiB->mWeights[weightIndex];

						if (aiW.mWeight == 0.f) {
							continue;
						}
						for (int jj = 0; jj < MAX_TOTAL_INFLUENCES; ++jj) {
							if (Weight.IsValidIndex(aiW.mVertexId + currentVertex) == false) {
								continue;
							}
							auto& s = Weight[aiW.mVertexId + currentVertex];
							if (s.InfluenceWeights[jj] > 0) {
								continue;
							}

							const float ww = FMath::Clamp(aiW.mWeight, 0.f, 1.f);
							if (ww < VRM4U_BoneWeightThreshold) {
								continue;
							}

							bonemap.AddUnique(b);
							AllActiveBones.AddUnique(b);
						}
					}
				}
				bonemap.Sort();


				for (uint32 boneIndex = 0; boneIndex < aiM->mNumBones; ++boneIndex) {
					auto &aiB = aiM->mBones[boneIndex];

					const int b = VRMGetRefSkeleton(sk).FindBoneIndex(UTF8_TO_TCHAR(aiB->mName.C_Str()));

					if (b < 0) {
						continue;
					}
					for (uint32 weightIndex = 0; weightIndex < aiB->mNumWeights; ++weightIndex) {
						auto &aiW = aiB->mWeights[weightIndex];

						if (aiW.mWeight == 0.f) {
							continue;
						}
						for (int jj = 0; jj < MAX_TOTAL_INFLUENCES; ++jj) {
							if (Weight.IsValidIndex(aiW.mVertexId + currentVertex) == false) {
								continue;
							}
							auto &s = Weight[aiW.mVertexId + currentVertex];
							if (s.InfluenceWeights[jj] > 0) {
								continue;
							}

							const float ww = FMath::Clamp(aiW.mWeight, 0.f, 1.f);
							if (ww < VRM4U_BoneWeightThreshold) {
								continue;
							}

							int tabledIndex = bonemap.AddUnique(b);
							if (tabledIndex == INDEX_NONE) {
								UE_LOG(LogVRM4ULoader, Warning, TEXT("bonemap add error!"));
								tabledIndex = 0;
							}

							if (tabledIndex > 255) {
								UE_LOG(LogVRM4ULoader, Warning, TEXT("bonemap over!"));
							}

							if (Options::Get().IsDebugOneBone()) {
								tabledIndex = 0;
							}

							s.InfluenceBones[jj] = tabledIndex;
							//s.InfluenceWeights[jj] = (VRM4U_BONE_INFLUENCT_TYPE)FMath::TruncToInt(ww * 255.f + (0.5f - KINDA_SMALL_NUMBER));
							s.InfluenceWeights[jj] = (VRM4U_BONE_INFLUENCE_TYPE)FMath::TruncToInt(ww * VRM4U_MaxBoneWeightFloat);

							meshWeight[aiW.mVertexId].InfluenceBones[jj] = s.InfluenceBones[jj];
							meshWeight[aiW.mVertexId].InfluenceWeights[jj] = s.InfluenceWeights[jj];

							if (Options::Get().IsMobileBone()){
								auto p = boneAll.FindByPredicate([&](BoneMapOpt &o) {return o.boneIndex == b; });
								if (p) {
									p->weight += aiW.mWeight;
								} else {
									BoneMapOpt o;
									o.boneIndex = b;
									o.weight = aiW.mWeight;
									boneAll.Add(o);
								}
							}

							break;
						}
					}
				}// bone loop

				// mobile remap
				if (Options::Get().IsMobileBone() && boneAll.Num() > 75) {
					TMap<int, int> mobileMap;

					auto bonemapNew = bonemap;

					while (boneAll.Num() > 75) {
						boneAll.Sort();

						// bone 0 == weight 0
						// search from 1
						auto &removed = boneAll[1];

						int findParent = removed.boneIndex;
						while (findParent >= 0) {
							findParent = VRMGetRefSkeleton(sk).GetParentIndex(findParent);
							auto p = boneAll.FindByPredicate([&](const BoneMapOpt &o) {return o.boneIndex == findParent;});

							if (p == nullptr) {
								continue;
							}

							p->weight += removed.weight;
							
							while(auto a = mobileMap.FindKey(removed.boneIndex) ){
								mobileMap[*a] = p->boneIndex;
							}
							mobileMap.FindOrAdd(removed.boneIndex) = p->boneIndex;
							break;
						}

						bonemapNew.Remove(removed.boneIndex);
						boneAll.RemoveAt(1);
					}
					if (mobileMap.Num()) {
						for (auto &a : Weight) {
							for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i) {
								auto &infBone = a.InfluenceBones[i];
								auto &infWeight = a.InfluenceWeights[i];
								if (bonemap.IsValidIndex(infBone) == false) {
									infWeight = 0.f;
									infBone = 0;
									continue;
								}
								auto srcBoneIndex = bonemap[infBone];
								auto f = mobileMap.Find(srcBoneIndex);
								if (f) {
									auto dstBoneIndex = *f;
									infBone = bonemapNew.IndexOfByKey(dstBoneIndex);
								} else {
									infBone = bonemapNew.IndexOfByKey(srcBoneIndex);
								}
							}
						}
						for (auto &a : meshWeight) {
							for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i) {
								auto &infBone = a.InfluenceBones[i];
								auto &infWeight = a.InfluenceWeights[i];
								if (bonemap.IsValidIndex(infBone) == false) {
									infWeight = 0.f;
									infBone = 0;
									continue;
								}
								auto srcBoneIndex = bonemap[infBone];
								auto f = mobileMap.Find(srcBoneIndex);
								if (f) {
									auto dstBoneIndex = *f;
									infBone = bonemapNew.IndexOfByKey(dstBoneIndex);
								} else {
									infBone = bonemapNew.IndexOfByKey(srcBoneIndex);
								}
							}
						}
					}
					bonemap = bonemapNew;
				}// mobile remap

				// normalize weight
				for (auto& w : meshWeight) {
					static int warnCount = 0;
					if (&w == &meshWeight[0]) {
						warnCount = 0;
					}

					// sort by Weight
					for (int i = 0; i < MAX_TOTAL_INFLUENCES - 1; ++i) {
						for (int j = i + 1; j < MAX_TOTAL_INFLUENCES; ++j) {
							if (w.InfluenceWeights[i] < w.InfluenceWeights[j]) {
								std::swap(w.InfluenceWeights[i], w.InfluenceWeights[j]);
								std::swap(w.InfluenceBones[i], w.InfluenceBones[j]);
							}
						}
					}

					int f = 0;
					int maxIndex = 0;
					int maxWeight = 0;
					for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i) {
						f += w.InfluenceWeights[i];

						if (maxWeight < w.InfluenceWeights[i]) {
							maxWeight = w.InfluenceWeights[i];
							maxIndex = i;
						}
					}
					if (f > VRM4U_MaxBoneWeight) {
						UE_LOG(LogVRM4ULoader, Warning, TEXT("overr"));
						w.InfluenceWeights[0] -= (VRM4U_BONE_INFLUENCE_TYPE)(f - VRM4U_MaxBoneWeight);
					}
					if (1) {
						if (f <= (VRM4U_MaxBoneWeight - MAX_TOTAL_INFLUENCES)) {
							if (warnCount < 50) {
								UE_LOG(LogVRM4ULoader, Warning, TEXT("less"));
								warnCount++;
							}
						}
						if (f == 0) {
							auto* p = GetNodeFromMeshID(meshID, aiData);

							int dummy = 0;
							if (p) {
								dummy = VRMGetRefSkeleton(sk).FindBoneIndex(UTF8_TO_TCHAR(p->mName.C_Str()));
								if (dummy == INDEX_NONE) {
									dummy = 0;
								}
								// add active bone for simple static mesh (not skinned mesh)
								AllActiveBones.AddUnique(dummy);
							}

							auto inf = bonemap.AddUnique(dummy);
							if (inf == INDEX_NONE) {
								inf = 0;
							}
							w.InfluenceBones[0] = inf;
							w.InfluenceWeights[0] += (VRM4U_BONE_INFLUENCE_TYPE)(VRM4U_MaxBoneWeight - f);

						} else {
							const int n = Options::Get().GetBoneWeightInfluenceNum();

							{
								// recalc weight
								f = 0;
								for (int i = 0; i < n; ++i) {
									f += w.InfluenceWeights[i];
								}
								if (f == 0) {
									f = 1;
								}
								for (int i = n; i < MAX_TOTAL_INFLUENCES; ++i) {
									w.InfluenceBones[i] = 0;
									w.InfluenceWeights[i] = 0;
								}

								int total = 0;
								for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i) {
									int t = w.InfluenceWeights[i];
									w.InfluenceWeights[i] = (VRM4U_BONE_INFLUENCE_TYPE)FMath::TruncToInt(VRM4U_MaxBoneWeightFloat * t / f);
									total += w.InfluenceWeights[i];
								}
								// adjust
								if (total > VRM4U_MaxBoneWeight) {
									UE_LOG(LogVRM4ULoader, Warning, TEXT("overr"));
									w.InfluenceWeights[0] -= (VRM4U_BONE_INFLUENCE_TYPE)(total - VRM4U_MaxBoneWeight);
								}
								w.InfluenceWeights[0] += (VRM4U_BONE_INFLUENCE_TYPE)(VRM4U_MaxBoneWeight - total);
							}
						}
					}
				}// nomalize weight

				if (VRMConverter::IsImportMode() == false) {
			
					FSkelMeshRenderSection &NewRenderSection = rd.RenderSections[meshID];

					bool bUseMergeMaterial = Options::Get().IsMergeMaterial();
					if ((int)aiM->mMaterialIndex >= vrmAssetList->MaterialMergeTable.Num()) {
						bUseMergeMaterial = false;
					}
					if (bUseMergeMaterial) {
						NewRenderSection.MaterialIndex = vrmAssetList->MaterialMergeTable[aiM->mMaterialIndex];// ModelSection.MaterialIndex;
					}else {
						NewRenderSection.MaterialIndex = aiM->mMaterialIndex;// ModelSection.MaterialIndex;
					}
					if (NewRenderSection.MaterialIndex >= vrmAssetList->Materials.Num()) NewRenderSection.MaterialIndex = 0;
					NewRenderSection.BaseIndex = currentIndex;
					NewRenderSection.NumTriangles = result.meshInfo[meshID].Triangles.Num() / 3;
					//NewRenderSection.bRecomputeTangent = ModelSection.bRecomputeTangent;
					NewRenderSection.bCastShadow = true;// ModelSection.bCastShadow;
					NewRenderSection.BaseVertexIndex = currentVertex;// currentVertex;// currentVertex;// ModelSection.BaseVertexIndex;
															//NewRenderSection.ClothMappingData = ModelSection.ClothMappingData;
															//NewRenderSection.BoneMap.SetNum(1);//ModelSection.BoneMap;
															//NewRenderSection.BoneMap[0] = 10;
					//NewRenderSection.BoneMap.SetNum(sk->Skeleton->GetBoneTree().Num());//ModelSection.BoneMap;
					//for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i) {
					//	NewRenderSection.BoneMap[i] = i;
					//}
					if (bonemap.Num() > 0) {
						NewRenderSection.BoneMap.SetNum(bonemap.Num());//ModelSection.BoneMap;
						for (int i = 0; i < NewRenderSection.BoneMap.Num(); ++i) {
							NewRenderSection.BoneMap[i] = bonemap[i];
						}
					}else {
						NewRenderSection.BoneMap.SetNum(1);
						auto *p = GetNodeFromMeshID(meshID, aiData);
						int32 i = k->GetReferenceSkeleton().FindBoneIndex(UTF8_TO_TCHAR(p->mName.C_Str()));
						if (i <= 0) {
							i = meshID;
						}
						NewRenderSection.BoneMap[0] = i;
					}


					NewRenderSection.NumVertices = result.meshInfo[meshID].Vertices.Num();// result.meshInfo[meshID].Triangles.Num();// allVertex;// result.meshInfo[meshID].Vertices.Num();// ModelSection.NumVertices;

					NewRenderSection.MaxBoneInfluences = Options::Get().GetBoneWeightInfluenceNum();// ModelSection.MaxBoneInfluences;
															//NewRenderSection.CorrespondClothAssetIndex = ModelSection.CorrespondClothAssetIndex;
															//NewRenderSection.ClothingData = ModelSection.ClothingData;
					TMap<int32, TArray<int32>> OverlappingVertices;
					NewRenderSection.DuplicatedVerticesBuffer.Init(NewRenderSection.NumVertices, OverlappingVertices);
					NewRenderSection.bDisabled = false;// ModelSection.bDisabled;
														//RenderSections.Add(NewRenderSection);
														//rd.RenderSections[0] = NewRenderSection;
				}



#if WITH_EDITORONLY_DATA
				{
					auto &s = sk->GetImportedModel()->LODModels[0].Sections[meshID];
					s.MaterialIndex = 0;

					TMap<int32, TArray<int32>> OverlappingVertices;

					{
						bool bUseMergeMaterial = Options::Get().IsMergeMaterial();
						if ((int)aiM->mMaterialIndex >= vrmAssetList->MaterialMergeTable.Num()) {
							bUseMergeMaterial = false;
						}
						if (bUseMergeMaterial) {
							s.MaterialIndex = vrmAssetList->MaterialMergeTable[aiM->mMaterialIndex];
						} else {
							s.MaterialIndex = aiM->mMaterialIndex;
						}
					}

					if (s.MaterialIndex >= vrmAssetList->Materials.Num()) s.MaterialIndex = 0;
#if	UE_VERSION_OLDER_THAN(4,24,0)
#else
					s.OriginalDataSectionIndex = meshID;
#endif
					s.BaseIndex = currentIndex;
					s.NumTriangles = result.meshInfo[meshID].Triangles.Num() / 3;
					s.BaseVertexIndex = currentVertex;
					s.SoftVertices = meshWeight;
					if (bonemap.Num() > 0) {
						s.BoneMap.SetNum(bonemap.Num());//ModelSection.BoneMap;
						for (int i = 0; i < s.BoneMap.Num(); ++i) {
							s.BoneMap[i] = bonemap[i];
						}
					}else {
						s.BoneMap.SetNum(1);
						auto *p = GetNodeFromMeshID(meshID, aiData);
						int32 i = k->GetReferenceSkeleton().FindBoneIndex(UTF8_TO_TCHAR(p->mName.C_Str()));
						if (i <= 0) {
							i = meshID;
						}
						s.BoneMap[0] = i;
					}
					s.NumVertices = meshWeight.Num();
					s.MaxBoneInfluences = Options::Get().GetBoneWeightInfluenceNum();
				}
#endif	// editor only data

				//rd.MultiSizeIndexContainer.CopyIndexBuffer(result.meshInfo[0].Triangles);
				int t1 = Triangles.Num();
				Triangles.Append(result.meshInfo[meshID].Triangles);
				int t2 = Triangles.Num();

				for (int i = t1; i < t2; ++i) {
					Triangles[i] += currentVertex;
				}
				currentIndex += result.meshInfo[meshID].Triangles.Num();
				currentVertex += result.meshInfo[meshID].Vertices.Num();
				//rd.MultiSizeIndexContainer.update
			} // mesh loop

			if (Options::Get().IsMergePrimitive()) {
#if WITH_EDITORONLY_DATA
				// merge lod model section
				auto &LodModel = sk->GetImportedModel()->LODModels[0];
				for (int meshID = LodModel.Sections.Num() - 1; meshID>0; --meshID) {
					auto &s0 = LodModel.Sections[meshID-1];
					auto &s1 = LodModel.Sections[meshID];

					if (aiData->mMeshes[meshID]->mNumAnimMeshes > 0) {
						// skip skin mesh
						continue;
					}
					if (s1.NumVertices == 0) {
						continue;
					}

					if (s0.MaterialIndex != s1.MaterialIndex) {
						continue;
					}
					auto newBoneMap = s0.BoneMap;
					for (auto &sv : s1.SoftVertices) {
						for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i) {
							if (sv.InfluenceWeights[i] == 0) {
								continue;
							}
							int boneID = s1.BoneMap[sv.InfluenceBones[i]];

							int ind = 0;
							if (newBoneMap.Find(boneID, ind)) {
								sv.InfluenceBones[i] = ind;
							} else {
								sv.InfluenceBones[i] = newBoneMap.Add(boneID);
							}
						}
					}
					if (Options::Get().IsMobileBone()) {
						if (newBoneMap.Num() > 75) {
							continue;
						}
					}

					s0.SoftVertices.Append(s1.SoftVertices);
					s0.NumVertices += s1.NumVertices;
					s0.NumTriangles += s1.NumTriangles;

					s0.BoneMap = newBoneMap;

					s1.SoftVertices.SetNum(0);
					s1.NumVertices = 0;
				}
				for (int meshID = 0; meshID < LodModel.Sections.Num(); ++meshID) {
					auto &s1 = LodModel.Sections[meshID];

					if (s1.NumVertices > 0) {
						continue;
					}
					LodModel.Sections.RemoveAt(meshID);
					meshID--;
				}
#endif
			} // merge primitive

			if (1) {
				int warnCount = 0;
				for (auto &w : Weight) {
					int f = 0;
					int maxIndex = 0;
					int maxWeight = 0;

#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
#if WITH_EDITOR
#else
					for (int i = 0; i < Options::Get().GetBoneWeightInfluenceNum(); ++i) {
						w.InfluenceWeights[i] = (VRM4U_BONE_INFLUENCE_TYPE)(((int)(w.InfluenceWeights[i] / 256)) * 256);
					}
#endif
#endif

					for (int i = 0; i < MAX_TOTAL_INFLUENCES; ++i) {
						f += w.InfluenceWeights[i];

						if (maxWeight < w.InfluenceWeights[i]) {
							maxWeight = w.InfluenceWeights[i];
							maxIndex = i;
						}
					}
					if (f > VRM4U_MaxBoneWeight) {
						UE_LOG(LogVRM4ULoader, Warning, TEXT("overr"));
						w.InfluenceWeights[0] -= (VRM4U_BONE_INFLUENCE_TYPE)(f - VRM4U_MaxBoneWeight);
					}
					if (f < VRM4U_MaxBoneWeight) {
						if (f <= (VRM4U_MaxBoneWeight - MAX_TOTAL_INFLUENCES)) {
							if (warnCount < 50) {
								UE_LOG(LogVRM4ULoader, Warning, TEXT("less"));
								warnCount++;
							}
						}
						w.InfluenceWeights[maxIndex] += (VRM4U_BONE_INFLUENCE_TYPE)(VRM4U_MaxBoneWeight - f);
					}
				}
			}

#if WITH_EDITOR
			{
				FSkeletalMeshLODModel *p = &(sk->GetImportedModel()->LODModels[0]);
				p->NumVertices = allVertex;
				p->NumTexCoords = uvNum; // allVertex;
				p->IndexBuffer = Triangles;
				p->ActiveBoneIndices = rd.ActiveBoneIndices;
				p->RequiredBones = rd.RequiredBones;
			}
#else // game

#if	UE_VERSION_OLDER_THAN(4,25,0)
#else
			// 4.25
			// force reinit renderdata
			{
				FSkeletalMeshRenderData* p = sk->GetResourceForRendering();
				auto *pRd = &(p->LODRenderData[0]);
				TArray<FSkinWeightInfo> InWeights;
				InWeights.SetNum(Weight.Num());

				int elem = 0;
				{
					if (Weight.Num()) {
						elem = std::extent<decltype(Weight[0].InfluenceBones)>::value;
					}
					FSkinWeightInfo info = {};
					elem = FMath::Min(elem, (int)std::extent<decltype(info.InfluenceBones)>::value);
				};

				for (int i = 0; i < Weight.Num(); ++i) {
					FSkinWeightInfo info = {};
					for (int al = 0; al < elem; ++al) {
						// uint8 <> uint16
						info.InfluenceBones[al] = Weight[i].InfluenceBones[al];
						info.InfluenceWeights[al] = Weight[i].InfluenceWeights[al];
					}
					InWeights[i] = info;
				}

				pRd->ReleaseResources();
				pRd->SkinWeightVertexBuffer = InWeights;
#if	UE_VERSION_OLDER_THAN(5,4,0)
				pRd->InitResources(false, 0, VRMGetMorphTargets(sk), sk);
#else
				{
					TArray<UMorphTarget*> dummy;
					for (auto& a : VRMGetMorphTargets(sk)) {
						dummy.Add(a);
					}
#if UE_VERSION_OLDER_THAN(5,7,0)
					pRd->InitResources(false, 0, dummy, sk);
#else
					pRd->InitResources(false, 0, sk);
#endif
				}
#endif // 5.4
			}
#endif // 4.25

#endif // editor

			//rd.StaticVertexBuffers.StaticMeshVertexBuffer.TexcoordDataPtr;

			if (VRMConverter::IsImportMode() == false) {
				ENQUEUE_RENDER_COMMAND(UpdateCommand)(
					[sk, Triangles, Weight](FRHICommandListImmediate& RHICmdList)
				{

#if	UE_VERSION_OLDER_THAN(5,3,0)
#define VRM4U_RHI_CMD_LIST
#else
#define VRM4U_RHI_CMD_LIST RHICmdList
#endif

					FSkeletalMeshLODRenderData &d = sk->GetResourceForRendering()->LODRenderData[0];

					if (d.MultiSizeIndexContainer.IsIndexBufferValid()) {
						d.MultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
					}
					d.MultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
					d.MultiSizeIndexContainer.GetIndexBuffer()->InitResource(VRM4U_RHI_CMD_LIST);

					//d.AdjacencyMultiSizeIndexContainer.CopyIndexBuffer(Triangles);
#if	UE_VERSION_OLDER_THAN(5,0,0)
					if (d.AdjacencyMultiSizeIndexContainer.IsIndexBufferValid()) {
						d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->ReleaseResource();
					}
					d.AdjacencyMultiSizeIndexContainer.RebuildIndexBuffer(sizeof(uint32), Triangles);
					d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->InitResource();
#endif

#if WITH_EDITOR
					d.SkinWeightVertexBuffer.Init(Weight);
#else

					{
#if	UE_VERSION_OLDER_THAN(4,25,0)
						typedef TSkinWeightInfo<false> WeightInfo;
#else
						typedef FSkinWeightInfo WeightInfo;
#endif
						TArray< WeightInfo > InWeights;
						InWeights.Reserve(Weight.Num());

						int elem = 0;
						{
							if (Weight.Num()) {
								elem = std::extent<decltype(Weight[0].InfluenceBones)>::value;
							}
							WeightInfo info = {};
							elem = FMath::Min(elem, (int)std::extent<decltype(info.InfluenceBones)>::value);
						};

						for (int i = 0; i < Weight.Num(); ++i) {
							auto n = new(InWeights) WeightInfo;

							WeightInfo info = {};
							for (int al = 0; al < elem; ++al) {
								// uint8 <> uint16
								info.InfluenceBones[al] = Weight[i].InfluenceBones[al];
								info.InfluenceWeights[al] = Weight[i].InfluenceWeights[al];
							}
							*n = info;
						}
						d.SkinWeightVertexBuffer = InWeights;
					}
#endif	// editor

#if	UE_VERSION_OLDER_THAN(4,25,0)
					d.SkinWeightVertexBuffer.InitResource();
#else
#endif

					d.StaticVertexBuffers.PositionVertexBuffer.UpdateRHI(VRM4U_RHI_CMD_LIST);
					d.StaticVertexBuffers.StaticMeshVertexBuffer.UpdateRHI(VRM4U_RHI_CMD_LIST);

					d.MultiSizeIndexContainer.GetIndexBuffer()->UpdateRHI(VRM4U_RHI_CMD_LIST);
#if	UE_VERSION_OLDER_THAN(5,0,0)
					d.AdjacencyMultiSizeIndexContainer.GetIndexBuffer()->UpdateRHI();
#endif

#if	UE_VERSION_OLDER_THAN(4,25,0)
					d.SkinWeightVertexBuffer.UpdateRHI();
#else
#endif
				});
			}
		}
		{

			FBox BoundingBox(BoundMin, BoundMax);
			sk->SetImportedBounds(FBoxSphereBounds(BoundingBox));
		}

		{
#if WITH_EDITOR
			{
				sk->GetImportedModel()->LODModels[0].NumTexCoords = uvNum;
			}
			if (VRMConverter::IsImportMode()) {
				sk->UpdateUVChannelData(false);
			}
#endif
		}

#if WITH_EDITOR
		if (VRMConverter::IsImportMode()) {

#if	UE_VERSION_OLDER_THAN(4,25,0)
			UProperty* ChangedProperty = FindField<UProperty>(USkeletalMesh::StaticClass(), "Materials");
			check(ChangedProperty);
			sk->PreEditChange(ChangedProperty);

			FPropertyChangedEvent PropertyUpdateStruct(ChangedProperty);
			sk->PostEditChangeProperty(PropertyUpdateStruct);

			sk->PostEditChange();
#else
			FProperty* ChangedProperty = FindFProperty<FProperty>(USkeletalMesh::StaticClass(), TEXT("Materials"));
			check(ChangedProperty);
			sk->PreEditChange(ChangedProperty);

			FPropertyChangedEvent PropertyUpdateStruct(ChangedProperty);
			sk->PostEditChangeProperty(PropertyUpdateStruct);

			sk->PostEditChange();
#endif
		}
#endif


#if WITH_EDITOR
		{
			//FSkeletalMeshLODRenderData& rd = sk->GetResourceForRendering()->LODRenderData[0];

			{
				FSkeletalMeshLODModel* p = &(sk->GetImportedModel()->LODModels[0]);
				if (Options::Get().IsActiveBone()) {
					auto& r = VRMGetSkeleton(sk)->GetReferenceSkeleton();
					for (int i = 0; i < AllActiveBones.Num(); ++i) {
						auto boneIndex = r.GetParentIndex(AllActiveBones[i]);
						if (boneIndex >= 0) {
							AllActiveBones.AddUnique(boneIndex);
						}
					}
					AllActiveBones.Sort();

					p->ActiveBoneIndices.SetNum(AllActiveBones.Num());
					for (int i = 0; i < AllActiveBones.Num(); ++i) {
						p->ActiveBoneIndices[i] = AllActiveBones[i];
					}
				} else {
					int boneNum = VRMGetSkeleton(sk)->GetReferenceSkeleton().GetRawBoneNum();

					p->ActiveBoneIndices.SetNum(boneNum);
					for (int i = 0; i < boneNum; ++i) {
						p->ActiveBoneIndices[i] = i;
					}
				}
			}
			{
				//p->NumVertices = allVertex;
				//p->NumTexCoords = uvNum; // allVertex;
				//p->IndexBuffer = Triangles;
				//p->RequiredBones = rd.RequiredBones;
			}
		}
#endif

	}

	UPhysicsAsset *pa = nullptr;
	{
		// remove
		if (vrmAssetList->Package == GetTransientPackage()) {
		} else {
			TArray<UObject*> ret;
			FString name = (TEXT("PHYS_") + vrmAssetList->BaseFileName);
			GetObjectsWithOuter(vrmAssetList->Package, ret);
			for (auto *a : ret) {
				auto s = a->GetName().ToLower();
				if (s.IsEmpty()) continue;

				if (s == name.ToLower()) {
					a->ClearFlags(EObjectFlags::RF_Standalone);
					a->SetFlags(EObjectFlags::RF_Public | RF_Transient);
					a->ConditionalBeginDestroy();
					break;
				}
			}
		}
	}
	if (aiData->mVRMMeta && Options::Get().IsSkipPhysics()==false) {

		bool spring0 = false;
		bool spring1 = false;

		VRM::VRMMetadata* meta = reinterpret_cast<VRM::VRMMetadata*>(aiData->mVRMMeta);
		// vrm0
		if (meta->springNum > 0) {
			spring0 = true;
		}

		if (vrmAssetList) {
			if (vrmAssetList->VrmMetaObject) {
				if (vrmAssetList->VrmMetaObject->VRM1SpringBoneMeta.Colliders.Num() > 0) {
					spring1 = true;
				}
			}
		}

		if (spring0 || spring1) {
			if (vrmAssetList->Package == GetTransientPackage()) {
				pa = VRM4U_NewObject<UPhysicsAsset>(GetTransientPackage(), NAME_None, EObjectFlags::RF_Public | RF_Transient, NULL);
			}
			else {
				FString name = (TEXT("PHYS_") + vrmAssetList->BaseFileName);
				pa = VRM4U_NewObject<UPhysicsAsset>(vrmAssetList->Package, *name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
			}
			pa->Modify();
#if WITH_EDITORONLY_DATA
			pa->SetPreviewMesh(sk);
#endif
			VRMSetPhysicsAsset(sk, pa);

			addedList.Empty();
		}

		// vrm0
		if (spring0) {
			TArray<int> swingBoneIndexArray;

			for (int i = 0; i < meta->springNum; ++i) {
				auto& spring = meta->springs[i];
				for (int j = 0; j < spring.boneNum; ++j) {
					auto& sbone = spring.bones_name[j];

					FString s = UTF8_TO_TCHAR(sbone.C_Str());

					int sboneIndex = VRMGetRefSkeleton(sk).FindRawBoneIndex(*s);
					if (sboneIndex == INDEX_NONE) {
						continue;
					}

					FName parentName = *s;
					CreateSwingHead(vrmAssetList, spring, parentName, swingBoneIndexArray, sboneIndex);

				}
			} // all spring
			{
				swingBoneIndexArray.Sort();

				for (int i = 0; i < swingBoneIndexArray.Num() - 1; ++i) {
					for (int j = i + 1; j < swingBoneIndexArray.Num(); ++j) {
						pa->DisableCollision(swingBoneIndexArray[i], swingBoneIndexArray[j]);
					}
				}
			}

			// collision
			for (int i = 0; i < meta->colliderGroupNum; ++i) {
				auto& c = meta->colliderGroups[i];

				USkeletalBodySetup* bs = nullptr;
				{
					FString s = UTF8_TO_TCHAR(c.node_name.C_Str());
					{
						int sboneIndex = VRMGetRefSkeleton(sk).FindRawBoneIndex(*s);
						if (sboneIndex == INDEX_NONE) {
							continue;
						}
					}
					s = s.ToLower();

					// addlist changed recur...
					if (1) {//addedList.Find(s) >= 0) {
						for (auto& a : pa->SkeletalBodySetups) {
							if (a->BoneName.IsEqual(*s)) {
								bs = a;
								break;
							}
						}

					}
					else {
						addedList.Add(s);
					}
				}
				if (bs == nullptr) {
					bs = NewObject<USkeletalBodySetup>(pa, NAME_None, RF_Transactional);
					bs->InvalidatePhysicsData();

					pa->SkeletalBodySetups.Add(bs);
				}

				FKAggregateGeom agg;
				for (int j = 0; j < c.colliderNum; ++j) {
					FKSphereElem SphereElem;
					VRM::vec3 v = {
						c.colliders[j].offset[0] * 100.f,
						c.colliders[j].offset[1] * 100.f,
						c.colliders[j].offset[2] * 100.f,
					};

					SphereElem.Center = FVector(-v[0], v[2], v[1]);
					SphereElem.Radius = c.colliders[j].radius * 100.f;
					agg.SphereElems.Add(SphereElem);
				}

				bs->Modify();
				bs->BoneName = UTF8_TO_TCHAR(c.node_name.C_Str());
				bs->AddCollisionFrom(agg);
				bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
				// newly created bodies default to simulating
				bs->PhysicsType = PhysType_Kinematic;	// fix!

				//bs.constra
				//sk->SkeletalBodySetups.Num();
				bs->CreatePhysicsMeshes();

			}// collision
		}// vrm0

		if (spring1){
			auto& sm = vrmAssetList->VrmMetaObject->VRM1SpringBoneMeta;

			TArray<int> swingBoneIndexArray;

			for (int i = 0; i < sm.Springs.Num(); ++i) {
				auto& spring = sm.Springs[i];

				for (int j = 0; j < spring.joints.Num(); ++j) {
					auto& sbone = spring.joints[j].boneName;

					FString s = sbone;

					int sboneIndex = VRMGetRefSkeleton(sk).FindRawBoneIndex(*s);
					if (sboneIndex == INDEX_NONE) {
						continue;
					}

					FName parentName = *s;
					//CreateSwingHead(vrmAssetList, spring, parentName, swingBoneIndexArray, sboneIndex);

				}
			} // all spring
			{
				swingBoneIndexArray.Sort();

				for (int i = 0; i < swingBoneIndexArray.Num() - 1; ++i) {
					for (int j = i + 1; j < swingBoneIndexArray.Num(); ++j) {
						pa->DisableCollision(swingBoneIndexArray[i], swingBoneIndexArray[j]);
					}
				}
			}

			// collision
			for (int i = 0; i < sm.ColliderGroups.Num(); ++i) {
				auto& c = sm.ColliderGroups[i];

				USkeletalBodySetup* bs = nullptr;
				/*
				{
					FString s = c.name;
					{
						int sboneIndex = VRMGetRefSkeleton(sk).FindRawBoneIndex(*s);
						if (sboneIndex == INDEX_NONE) {
							continue;
						}
					}
					s = s.ToLower();

					// addlist changed recur...
					if (1) {//addedList.Find(s) >= 0) {
						for (auto& a : pa->SkeletalBodySetups) {
							if (a->BoneName.IsEqual(*s)) {
								bs = a;
								break;
							}
						}

					}
					else {
						addedList.Add(s);
					}
				}
				*/

				for (int j = 0; j < c.colliders.Num(); ++j) {

					FKAggregateGeom agg;

					int ind = c.colliders[j];
					auto& col = sm.Colliders[ind];


					// sb setup
					{
						FString s = col.boneName;
						{
							int sboneIndex = VRMGetRefSkeleton(sk).FindRawBoneIndex(*s);
							if (sboneIndex == INDEX_NONE) {
								continue;
							}
						}
						s = s.ToLower();

						// addlist changed recur...
						if (1) {//addedList.Find(s) >= 0) {
							for (auto& a : pa->SkeletalBodySetups) {
								if (a->BoneName.IsEqual(*s)) {
									bs = a;
									break;
								}
							}

						}
						else {
							addedList.Add(s);
						}
					}
					if (bs == nullptr) {
						bs = NewObject<USkeletalBodySetup>(pa, NAME_None, RF_Transactional);
						bs->InvalidatePhysicsData();

						pa->SkeletalBodySetups.Add(bs);
					}


					if (col.shapeType == TEXT("sphere")) {
						VRM::vec3 v = {
							col.offset[0] * 100,
							col.offset[1] * 100,
							col.offset[2] * 100
						};

						FKSphereElem SphereElem;
						SphereElem.Center = FVector(v[0], -v[2], v[1]);
						SphereElem.Radius = col.radius * 100.f;
						agg.SphereElems.Add(SphereElem);
					}
					else {
						VRM::vec3 v0 = {
							col.offset[0] * 100,
							col.offset[1] * 100,
							col.offset[2] * 100
						};

						VRM::vec3 v1 = {
							col.tail[0] * 100,
							col.tail[1] * 100,
							col.tail[2] * 100
						};
						VRM::vec3 v = {
							v0[0] + v1[0] / 2,
							v0[1] + v1[1] / 2,
							v0[2] + v1[2] / 2,
						};

						float len =
							FMath::Sqrt(
								FMath::Square(v0[0] - v1[0])
								+ FMath::Square(v0[1] - v1[1])
								+ FMath::Square(v0[2] - v1[2])
							);


						FKSphylElem s;
						s.Center = FVector(v[0], -v[2], v[1]);
						s.Radius = col.radius * 100.f;
						s.Length = len;
						agg.SphylElems.Add(s);
					}
					bs->Modify();
					bs->BoneName = *col.boneName;
					bs->AddCollisionFrom(agg);
					bs->CollisionTraceFlag = CTF_UseSimpleAsComplex;
					// newly created bodies default to simulating
					bs->PhysicsType = PhysType_Kinematic;	// fix!

					//bs.constra
					//sk->SkeletalBodySetups.Num();
					bs->CreatePhysicsMeshes();
				}// collision
			}// collision group
		}

		if (pa){
			pa->UpdateBodySetupIndexMap();

			RefreshSkelMeshOnPhysicsAssetChange(sk);
#if WITH_EDITOR
			pa->RefreshPhysicsAssetChange();
#endif
			pa->UpdateBoundsBodiesArray();

#if WITH_EDITOR
			if (VRMConverter::IsImportMode()) {
				pa->PostEditChange();
			}
#endif
		}
		//FSkeletalMeshModel* ImportedResource = sk->GetImportedModel();
		//if (ImportedResource->LODModels.Num() == 0)
	}

	//NewAsset->FindSocket


#if WITH_EDITOR
	// animation
	if (aiData->mNumAnimations > 0) {
		UAnimSequence* ase;
		ase = VRM4U_NewObject<UAnimSequence>(vrmAssetList->Package, *(TEXT("A_") + vrmAssetList->BaseFileName), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		ase->SetSkeleton(k);

#if	UE_VERSION_OLDER_THAN(5,0,0)
		ase->CleanAnimSequenceForImport();
#else


		int FrameNum = 0;
		for (uint32_t animNo = 0; animNo < aiData->mNumAnimations; animNo++) {
			aiAnimation* aiA = aiData->mAnimations[animNo];

			for (uint32_t chanNo = 0; chanNo < aiA->mNumChannels; chanNo++) {
				aiNodeAnim* aiNA = aiA->mChannels[chanNo];

				FrameNum = FMath::Max(FrameNum, (int)aiNA->mNumPositionKeys);
				FrameNum = FMath::Max(FrameNum, (int)aiNA->mNumRotationKeys);
			}
		}
		auto& Controller = ase->GetController();
#if	UE_VERSION_OLDER_THAN(5,2,0)
#else
		Controller.OpenBracket(LOCTEXT("VRM4U", "Importing BVH"), false);
#endif
		Controller.ResetModel();

		double AnimDeltaTime = 0.f;
		{
			for (uint32_t animNo = 0; animNo < aiData->mNumAnimations; animNo++) {
				aiAnimation* aiA = aiData->mAnimations[animNo];

				for (uint32_t chanNo = 0; chanNo < aiA->mNumChannels; chanNo++) {
					aiNodeAnim* aiNA = aiA->mChannels[chanNo];

					for (int i = 0; i < (int)(aiNA->mNumRotationKeys)-1; ++i) {
						auto dif = (aiNA->mRotationKeys[i+1].mTime - aiNA->mRotationKeys[i].mTime) / aiA->mTicksPerSecond;
						if (AnimDeltaTime == 0.f && dif != 0.f) {
							AnimDeltaTime = dif;
						}
					}
				}
			}
		}


#if	UE_VERSION_OLDER_THAN(5,2,0)
		Controller.SetPlayLength(FrameNum-1);
		Controller.SetFrameRate(FFrameRate(1, 1));
#else
		Controller.InitializeModel();
		ase->ResetAnimation();
		ase->SetPreviewMesh(sk);
		if (AnimDeltaTime > 0.f) {
			Controller.SetFrameRate(FFrameRate(1.f / AnimDeltaTime, 1));
		} else {
			Controller.SetFrameRate(FCommonFrameRates::FPS_30());
		}
		Controller.SetNumberOfFrames(FrameNum - 1);
#endif

#if	UE_VERSION_OLDER_THAN(5,3,0)
		Controller.UpdateCurveNamesFromSkeleton(k, ERawCurveTrackTypes::RCT_Float);
#endif

		//Controller.OpenBracket(LOCTEXT("VRM4U", "Importing BVH"), false);

		ase->RateScale = VRMConverter::Options::Get().GetAnimationPlayRateScale();
#endif



		float totalTime = 0.f;
		int totalFrameNum = 0;
		if (1){

			//TArray<AnimationTransformDebug::FAnimationTransformDebugData> TransformDebugData;

			for (uint32_t animNo = 0; animNo < aiData->mNumAnimations; animNo++) {
				aiAnimation* aiA = aiData->mAnimations[animNo];

				for (uint32_t chanNo = 0; chanNo < aiA->mNumChannels; chanNo++) {
					aiNodeAnim* aiNA = aiA->mChannels[chanNo];

					FRawAnimSequenceTrack RawTrack;

					bool isRootBone = false;
					FName NodeName = UTF8_TO_TCHAR(aiNA->mNodeName.C_Str());
					if (VRMConverter::Options::Get().IsVRMAModel()) {
						if (vrmAssetList->VrmMetaObject) {
							for (auto& t : vrmAssetList->VrmMetaObject->humanoidBoneTable) {

								if (t.Value == NodeName.ToString()) {
									NodeName = *t.Key;
									break;
								}
							}
						}
					}
					{
						auto ind = k->GetReferenceSkeleton().FindBoneIndex(NodeName);
						if (ind != INDEX_NONE) {
							ind = k->GetReferenceSkeleton().GetParentIndex(ind);
							if (ind == INDEX_NONE) {
								// root bone. no parent.

								isRootBone = true;
							}
						}
					}

					{
						for (uint32_t i = 0; i < aiNA->mNumPositionKeys; ++i) {
							const auto &v = aiNA->mPositionKeys[i].mValue;
							//FVector pos(v.x, v.y, v.z);
							float Scale = 1.f;
							if (VRMConverter::Options::Get().IsVRMAModel()) {
								Scale = 100.f;
							}
							FVector pos(-v.x, v.z, v.y);
							pos *= Scale * VRMConverter::Options::Get().GetAnimationTranslateScale();
							if (VRMConverter::Options::Get().IsVRM10Model()) {
								pos.Set(v.x, -v.z, v.y);
								pos *= Scale * VRMConverter::Options::Get().GetAnimationTranslateScale();
							}
							if (VRMConverter::Options::Get().IsPMXModel() || VRMConverter::Options::Get().IsBVHModel()) {
								pos.X *= -1.f;
								pos.Y *= -1.f;
							}
							//if (VRMConverter::Options::Get().IsVRMAModel() ) {
							//	if (isRootBone) {
							//		pos.X *= -1.f;
							//		pos.Y *= -1.f;
							//	}else {
							//		pos.Set(v.x, v.y, v.z);
							//		pos *= Scale * VRMConverter::Options::Get().GetAnimationTranslateScale();
							//	}
							//}
#if UE_VERSION_OLDER_THAN(5,0,0)
							RawTrack.PosKeys.Add(pos);
#else
							RawTrack.PosKeys.Add(FVector3f(pos));
#endif

							totalTime = FMath::Max((float)aiNA->mPositionKeys[i].mTime, totalTime);
						}
						if (chanNo == 0) {
							if (RawTrack.PosKeys.Num()) {
								//ase->bEnableRootMotion = true;
							}
						}
					}

					{
						for (uint32_t i = 0; i < aiNA->mNumRotationKeys; ++i) {
							const auto &v = aiNA->mRotationKeys[i].mValue;

#if UE_VERSION_OLDER_THAN(5,0,0)
							FQuat q(v.x, v.y, v.z, v.w);
#else
							FQuat4f q(v.x, v.y, v.z, v.w);
#endif
							

							if (1) {
#if UE_VERSION_OLDER_THAN(5,0,0)
								q = FQuat(-v.x, v.y, v.z, -v.w);
								{
									FQuat d = FQuat(FVector(1, 0, 0), -PI / 2.f);
									q = d * q * d.Inverse();
								}
#else
								q = FQuat4f(-v.x, v.y, v.z, -v.w);

								if (VRMConverter::Options::Get().IsBVHModel()) {
									auto d = FQuat4f(FVector3f(1, 0, 0), -PI / 2.f);
									q = d * q * d.Inverse();
								}

								if (VRMConverter::Options::Get().IsVRM10Model()) {
									q = FQuat4f(v.x, -v.z, v.y, v.w);
								}
#endif

							}

							//FQuat q(v.x, v.y, v.z, v.w);
							//FVector a = q.GetRotationAxis();
							//q = FQuat(FVector(0, 0, 1), PI) * q;
							//FMatrix m = q.GetRotationAxis
							//q = FRotator(0, 90, 0).Quaternion() * q;
							RawTrack.RotKeys.Add(q);

							totalTime = FMath::Max((float)aiNA->mRotationKeys[i].mTime, totalTime);
						}

						for (uint32_t i = 0; i < aiNA->mNumScalingKeys; ++i) {
							const auto &v = aiNA->mScalingKeys[i].mValue;
							FVector s(v.x, v.y, v.z);
#if UE_VERSION_OLDER_THAN(5,0,0)
							RawTrack.ScaleKeys.Add(s);
#else
							RawTrack.ScaleKeys.Add(FVector3f(s));
#endif

							totalTime = FMath::Max((float)aiNA->mScalingKeys[i].mTime, totalTime);
						}
					}

					if (RawTrack.RotKeys.Num() || RawTrack.PosKeys.Num() || RawTrack.ScaleKeys.Num()) {


#if UE_VERSION_OLDER_THAN(5,0,0)
						if (RawTrack.PosKeys.Num() == 0) {
							RawTrack.PosKeys.Add(FVector::ZeroVector);
						}
						if (RawTrack.RotKeys.Num() == 0) {
							RawTrack.RotKeys.Add(FQuat::Identity);
						}
						if (RawTrack.ScaleKeys.Num() == 0) {
							RawTrack.ScaleKeys.Add(FVector::OneVector);
						}
#else
						while (RawTrack.PosKeys.Num() < FrameNum) {
							RawTrack.PosKeys.Add(FVector3f::ZeroVector);
						}
						while (RawTrack.RotKeys.Num() < FrameNum) {
							RawTrack.RotKeys.Add(FQuat4f::Identity);
						}
						while (RawTrack.ScaleKeys.Num() < FrameNum) {
							RawTrack.ScaleKeys.Add(FVector3f::OneVector);
						}
#endif

#if	UE_VERSION_OLDER_THAN(5,0,0)
						int32 NewTrackIdx = ase->AddNewRawTrack(UTF8_TO_TCHAR(aiNA->mNodeName.C_Str()), &RawTrack);
#elif UE_VERSION_OLDER_THAN(5,2,0)
						if (Controller.AddBoneTrack(UTF8_TO_TCHAR(aiNA->mNodeName.C_Str())) != INDEX_NONE) {
							Controller.SetBoneTrackKeys(UTF8_TO_TCHAR(aiNA->mNodeName.C_Str()), RawTrack.PosKeys, RawTrack.RotKeys, RawTrack.ScaleKeys);
						}
#else
						{
							if (VRMConverter::Options::Get().IsVRMAModel()) {
								auto &presetList = vrmAssetList->VrmMetaObject->VRMAnimationMeta.expressionPreset;
								for (auto& p : presetList) {
									if (NodeName != *p.expressionNodeName) continue;

									FFloatCurve c;
									c.SetCurveTypeFlag(AACF_Editable, true);


									auto boneNo = k->GetReferenceSkeleton().FindBoneIndex(*p.expressionNodeName);
									if (boneNo != INDEX_NONE) {

										for (int i = 0; i < RawTrack.PosKeys.Num(); ++i) {
											c.UpdateOrAddKey(RawTrack.PosKeys[i].X / 100.f, i * AnimDeltaTime);
										}
									}
#if	UE_VERSION_OLDER_THAN(5,3,0)
									FSmartName sm;
									{
										TArray<FName> a = {*p.expressionName };
										k->RemoveSmartnamesAndModify(USkeleton::AnimCurveMappingName, a);
									}
									k->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, *p.expressionName, sm);
									FAnimationCurveIdentifier f(sm, ERawCurveTrackTypes::RCT_Float);
#else
									FAnimationCurveIdentifier f(*p.expressionName, ERawCurveTrackTypes::RCT_Float);
#endif
									FName name;
									Controller.AddCurve(f);
									Controller.SetCurveKeys(f, c.FloatCurve.GetConstRefOfKeys());
								}
							}

							if (Controller.AddBoneCurve(NodeName)) {
								Controller.SetBoneTrackKeys(NodeName, RawTrack.PosKeys, RawTrack.RotKeys, RawTrack.ScaleKeys);
							}
						}
#endif


					}

					totalFrameNum = FMath::Max(totalFrameNum, RawTrack.RotKeys.Num());
					totalFrameNum = FMath::Max(totalFrameNum, RawTrack.PosKeys.Num());
					totalTime = totalFrameNum / aiA->mTicksPerSecond;

#if	UE_VERSION_OLDER_THAN(4,22,0)
					ase->NumFrames = totalFrameNum;
#elif UE_VERSION_OLDER_THAN(5,0,0)
					ase->SetRawNumberOfFrame(totalFrameNum);
#else
#endif
				}
			}
		}

		{
#if UE_VERSION_OLDER_THAN(5,0,0)
			ase->SequenceLength = totalTime;
			const bool bSourceDataExists = ase->HasSourceRawData();
			if (bSourceDataExists)
			{
				ase->BakeTrackCurvesToRawAnimation();
			} else {
				ase->PostProcessSequence();
			}
#else
			Controller.NotifyPopulated();
			Controller.CloseBracket(true);
#endif

#if UE_VERSION_OLDER_THAN(5,2,0)
			ase->MarkRawDataAsModified();
#else
#endif

		}
		ase->PostEditChange();
	}

	{
		FSoftObjectPath r(TEXT("/VRM4U/Util/Actor/latest/ABP_PostProcessBase.ABP_PostProcessBase"));
		UObject* u = r.TryLoad();

		if (u) {
			FString name = FString(TEXT("ABP_Post_")) + vrmAssetList->BaseFileName;

			auto b = Cast<UAnimBlueprint>(VRM4U_StaticDuplicateObject(u, vrmAssetList->Package, *name, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone));
			if (b) {
				b->MarkPackageDirty();
				b->TargetSkeleton = k;
				b->SetPreviewMesh(sk);
#if WITH_EDITOR
				FKismetEditorUtilities::CompileBlueprint(b);
#endif
				UBlueprintGeneratedClass* bpClass = Cast<UBlueprintGeneratedClass>(b->GeneratedClass);
#if	UE_VERSION_OLDER_THAN(4,27,0)
				sk->PostProcessAnimBlueprint = bpClass;
#else
				sk->SetPostProcessAnimBlueprint(bpClass);
#endif
				sk->PostEditChange();
			}
		}
	}
#endif

	return true;
}


VrmConvertModel::VrmConvertModel()
{
}

VrmConvertModel::~VrmConvertModel()
{
}
