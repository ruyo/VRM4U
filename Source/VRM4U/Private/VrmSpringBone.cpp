#include "VrmSpringBone.h"

#include "VrmAssetListObject.h"
#include "Engine/World.h"

VrmSpringBone::VrmSpringBone()
{
}

VrmSpringBone::~VrmSpringBone()
{
}

namespace VRMSpringBone {

	void VRMSpring::Update(const FAnimNode_VrmSpringBone* animNode, float DeltaTime, FTransform ComponentToLocal,
		const TArray<VRMSpringColliderGroup>& colliderGroup,
		FComponentSpacePoseContext& Output) {

		if (skeletalMesh == nullptr) {
			return;
		}

		const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
		const auto& RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();

		const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
		// モデルローカル座標
		ComponentToLocal = ComponentTransform.Inverse();

		//
		// x10 adjust?
		FVector ue4grav(-gravityDir.X, gravityDir.Z, gravityDir.Y);

		const int MAX_LOOP = FMath::Max(1, animNode->loopc);
		for (int i = 0; i < MAX_LOOP; ++i) {
			//const float stiffnessForce = stiffness * DeltaTime * 10.f * animNode->stiffnessScale + animNode->stiffinessAdd;
			//FVector external = ComponentToLocal.TransformVector(ue4grav) * (gravityPower * DeltaTime) * animNode->gravityScale + ComponentToLocal.TransformVector(animNode->gravityAdd) * DeltaTime;
			//external *= 100.f; // to unreal scale

			float CurrentDeltaTime = DeltaTime / (float)MAX_LOOP;

			const float stiffnessForce = stiffness * CurrentDeltaTime * 10.f * animNode->stiffnessScale + animNode->stiffnessAdd;
			FVector external = ComponentToLocal.TransformVector(ue4grav) * (gravityPower * CurrentDeltaTime) * animNode->gravityScale + ComponentToLocal.TransformVector(animNode->gravityAdd) * CurrentDeltaTime;

			//wind
			if (animNode->bIgnoreWindDirectionalSource == false) {
				const USkeletalMeshComponent* SkelComp = Output.AnimInstanceProxy->GetSkelMeshComponent();
				//InAnimInstance->GetSkelMeshComponent();

				const UWorld* World = nullptr;
				if (SkelComp) {
					World = SkelComp->GetWorld();
				}
				FSceneInterface* Scene = nullptr;
				if (World) {
					Scene = World->Scene;
				}

				if (Scene)
				{

					// Unused by our simulation but needed for the call to GetWindParameters below
					float WindMinGust;
					float WindMaxGust;
					FVector WindDirection;
					float WindSpeed;


					// Setup wind data
					//Body->bWindEnabled = true;
					//Scene->GetWindParameters_GameThread(SkelComp->GetComponentTransform().TransformPosition(Body->Pose.Position), Body->WindData.WindDirection, Body->WindData.WindSpeed, WindMinGust, WindMaxGust);
					Scene->GetWindParameters_GameThread(SkelComp->GetComponentTransform().GetLocation(), WindDirection, WindSpeed, WindMinGust, WindMaxGust);

					WindDirection = SkelComp->GetComponentTransform().Inverse().TransformVector(WindDirection);

					// from AnimPhysicsSolver
					const float WindUnitScale = 0.5f * 250.0f * FMath::FRandRange(1.f - animNode->randomWindRange, 1.f + animNode->randomWindRange) * animNode->windScale;
					//

					// Wind velocity in body space
					FVector WindVelocity = WindDirection * WindSpeed * WindUnitScale;// *BodyWindScale;
					WindVelocity *= CurrentDeltaTime;

					//Body->WindData.WindDirection = SkelComp->GetComponentTransform().Inverse().TransformVector(Body->WindData.WindDirection);
					//Body->WindData.WindAdaption = FMath::FRandRange(0.0f, 2.0f);
					//Body->WindData.BodyWindScale = WindScale;

					//if (CVarEnableWind.GetValueOnAnyThread() == 1 && bEnableWind)

					external += WindVelocity / 100.f;
				}
			}// wind end


			external *= 100.f; // to unreal scale

			FVector external_noAdd = ComponentToLocal.TransformVector(ue4grav) * (gravityPower * CurrentDeltaTime) * animNode->gravityScale;
			external_noAdd *= 100.f; // to unreal scale


			const auto WorldContext = Output.AnimInstanceProxy->GetSkelMeshComponent();

			for (int springCount = 0; springCount < SpringDataChain.Num(); ++springCount) {

				auto& ChainRoot = SpringDataChain[springCount];
				FTransform currentTransform = FTransform::Identity;

				bool bSkipGravAdd = false;

				for (int chainCount = 0; chainCount < ChainRoot.Num(); ++chainCount) {

					auto& sData = ChainRoot[chainCount];

					FVector currentTail = ComponentToLocal.TransformPosition(sData.m_currentTail);
					FVector prevTail = ComponentToLocal.TransformPosition(sData.m_prevTail);

					int myBoneIndex = RefSkeleton.FindBoneIndex(sData.boneName);
					if (myBoneIndex < 0) {
						continue;
					}

					if (animNode->NoWindBoneNameList.Contains(sData.boneName)) {
						bSkipGravAdd = true;
					}


					int myParentBoneIndex = RefSkeleton.GetParentIndex(myBoneIndex);

					//currentTransform = FTransform::Identity;
					FQuat ParentRotation = FQuat::Identity;
					if (chainCount == 0) {
						FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(myBoneIndex);
						//FCompactPoseBoneIndex uu(myBoneIndex);

						if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
							continue;
						}

						FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(uu);
						ParentRotation = NewBoneTM.GetRotation();

						currentTransform = NewBoneTM;
					}
					else {
						auto c = RefSkeletonTransform[sData.boneIndex];
						auto t = c * currentTransform;

						ParentRotation = t.GetRotation();
						currentTransform = t;
					}
					FQuat m_localRotation = FQuat::Identity;


					// verlet積分で次の位置を計算
					FVector nextTail = currentTail
						+ (currentTail - prevTail) * (1.0f - dragForce) // 前フレームの移動を継続する(減衰もあるよ)
						+ ParentRotation * m_localRotation * sData.m_boneAxis * stiffnessForce // 親の回転による子ボーンの移動目標
						;
					if (bSkipGravAdd) {
						nextTail += external_noAdd; // 外力による移動量
					}
					else {
						nextTail += external; // 外力による移動量
					}


					// 長さをboneLengthに強制
					//nextTail = sData.m_transform.GetLocation() + (nextTail - sData.m_transform.GetLocation()).GetSafeNormal() * sData.m_length;
					nextTail = currentTransform.GetLocation() + (nextTail - currentTransform.GetLocation()).GetSafeNormal() * sData.m_length;

					// Collisionで移動

					// vrm <-> physics collision
					if (animNode->bIgnorePhysicsCollision == false) {
						const int ColCount = animNode->collisionCheckLoopCount;
						for (int colc = 0; colc < ColCount; ++colc) {
							FVector Start = ComponentToLocal.InverseTransformPosition(nextTail);
							FVector End = Start + FVector(0.001f);
							//* WorldContextObject
							float Radius = hitRadius * 100.f;
							ETraceTypeQuery TraceChannel = ETraceTypeQuery::TraceTypeQuery1;
							bool bTraceComplex = false;

							TArray<AActor*> ActorsToIgnore;
							EDrawDebugTrace::Type DrawDebugType = EDrawDebugTrace::None;
							TArray<FHitResult> OutHits;
							bool bIgnoreSelf = true;
							FLinearColor TraceColor(EForceInit::ForceInit);
							FLinearColor TraceHitColor(EForceInit::ForceInit);
							float DrawTime = 0.f;

							bool b = UKismetSystemLibrary::SphereTraceMulti(WorldContext, Start, End, hitRadius * 100.f,
								TraceChannel, false, ActorsToIgnore,
								DrawDebugType,
								OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
							if (b == false) {
								break;
							}
							for (auto hit : OutHits) {
								{
									//WorldContext->GetOwner
									//hit.BoneName

								}
								float r = hit.PenetrationDepth; //  hitRadius * 100.f + hit.Distance;
								auto normal = hit.Normal;
								auto posFromCollider = nextTail + normal * r; // (float)ColCount;
								// 長さをboneLengthに強制
								nextTail = currentTransform.GetLocation() + (posFromCollider - currentTransform.GetLocation()).GetSafeNormal() * sData.m_length;
							}
						}
					}

					// vrm <-> vrm collision
					if (animNode->bIgnoreVRMCollision == false) {
						for (auto ind : ColliderGroupIndexArray) {
							if (ind >= colliderGroup.Num()) {
								continue;
							}
							const auto& cg = colliderGroup[ind];

							int ii = RefSkeleton.FindBoneIndex(cg.node_name);
							if (ii < 0) {
								continue;
							}

							FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(ii);
							//FCompactPoseBoneIndex uu(ii);
							if (uu == INDEX_NONE) {
								continue;
							}
							FTransform collisionBoneTrans = Output.Pose.GetComponentSpaceTransform(uu);

							for (auto c : cg.colliders) {


								float r = (hitRadius + c.radius) * 100.f;
								//FVector v = collisionBoneTrans.TransformPosition(c.offset*100);
								auto offs = c.offset;
								//offs.Set(offs.X, -offs.Z, offs.Y);	// 本来はこれが正しいが、VRM0の座標が間違っている
								offs.Set(-offs.X, offs.Z, offs.Y);		// VRM0の仕様としては これ
								offs *= 100;
								FVector v = collisionBoneTrans.TransformPosition(offs);

								if ((v - nextTail).SizeSquared() > r * r) {
									continue;
								}

								// ヒット。Colliderの半径方向に押し出す
								auto normal = (nextTail - v).GetSafeNormal();
								auto posFromCollider = v + normal * (r);
								// 長さをboneLengthに強制
								nextTail = currentTransform.GetLocation() + (posFromCollider - currentTransform.GetLocation()).GetSafeNormal() * sData.m_length;
							}
						}
					}

					sData.m_prevTail = ComponentToLocal.InverseTransformPosition(currentTail);
					sData.m_currentTail = ComponentToLocal.InverseTransformPosition(nextTail);

					FQuat rotation = ParentRotation * m_localRotation;

					sData.m_resultQuat = FQuat::FindBetween((rotation * sData.m_boneAxis).GetSafeNormal(),
						(nextTail - currentTransform.GetLocation()).GetSafeNormal()) * rotation;

					currentTransform.SetRotation(sData.m_resultQuat);
				}
			}// chain loop
		}// delta time loop
	}


	void VRMSpringManager::reset() {
		spring.Empty();
		bInit = false;
	}

	void VRMSpringManager::init(const UVrmMetaObject* meta, FComponentSpacePoseContext& Output) {
		if (meta == nullptr) return;
		if (bInit) return;

		if (meta->GetVRMVersion() == 1) return;
		if (meta->VrmAssetListObject == nullptr) return;

		skeletalMesh = VRMGetSkinnedAsset(Output.AnimInstanceProxy->GetSkelMeshComponent());
		//skeletalMesh = meta->SkeletalMesh;
		const FReferenceSkeleton& RefSkeleton = VRMGetRefSkeleton(skeletalMesh);
		const auto& RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();

		spring.SetNum(meta->VRMSpringMeta.Num());

		for (int i = 0; i < spring.Num(); ++i) {
			auto& s = spring[i];
			const auto& metaS = meta->VRMSpringMeta[i];

			s.skeletalMesh = skeletalMesh;// meta->SkeletalMesh;

			s.stiffness = metaS.stiffness;
			s.gravityPower = metaS.gravityPower;
			s.gravityDir = meta->VrmAssetListObject->model_root_transform.TransformVector(metaS.gravityDir);
			s.dragForce = metaS.dragForce;
			s.hitRadius = metaS.hitRadius;

			s.RootSpringData.SetNum(metaS.bones.Num());
			for (int scount = 0; scount < s.RootSpringData.Num(); ++scount) {
				s.RootSpringData[scount].boneName = *metaS.boneNames[scount];
				s.RootSpringData[scount].boneIndex = RefSkeleton.FindBoneIndex(*metaS.boneNames[scount]);
			}
			// TODO add child

			s.SpringDataChain.SetNum(metaS.bones.Num());
			for (int scount = 0; scount < s.RootSpringData.Num(); ++scount) {
				auto& chain = s.SpringDataChain[scount];

				//root
				{
					auto index = VRMGetRefSkeleton(skeletalMesh).FindBoneIndex(*metaS.boneNames[scount]);
					if (index == INDEX_NONE) {
						continue;
					}

					auto& sData = chain.AddDefaulted_GetRef();
					sData.boneName = *metaS.boneNames[scount];
					sData.boneIndex = index;

					TArray<int32> Children;
					VRMUtil::GetDirectChildBones(VRMGetRefSkeleton(skeletalMesh), sData.boneIndex, Children);
					if (Children.Num() > 0) {
						sData.m_boneAxis = RefSkeletonTransform[Children[0]].GetLocation();
					}
					else {
						sData.m_boneAxis = RefSkeletonTransform[sData.boneIndex].GetLocation() * 0.7f;
					}
				}

				// child
				if (1) {
					for (int chainCount = 0; chainCount < 100; ++chainCount) {
						bool bLast = false;
						TArray<int32> Children;

						VRMUtil::GetDirectChildBones(VRMGetRefSkeleton(skeletalMesh), chain[chainCount].boneIndex, Children);
						if (Children.Num() <= 0) {
							break;
						}

						auto& sData = chain.AddDefaulted_GetRef();

						sData.boneIndex = Children[0];
						sData.boneName = *RefSkeleton.GetBoneName(sData.boneIndex).ToString();

						VRMUtil::GetDirectChildBones(VRMGetRefSkeleton(skeletalMesh), sData.boneIndex, Children);
						if (Children.Num() > 0) {
							sData.m_boneAxis = RefSkeletonTransform[Children[0]].GetLocation();
						}
						else {
							sData.m_boneAxis = RefSkeletonTransform[sData.boneIndex].GetLocation() * 0.7f;
						}
					}
				}
			}


			s.ColliderGroupIndexArray.SetNum(metaS.ColliderIndexArray.Num());
			for (int c = 0; c < s.ColliderGroupIndexArray.Num(); ++c) {
				s.ColliderGroupIndexArray[c] = metaS.ColliderIndexArray[c];
			}

		}

		// init default transform
		{
			const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
			for (auto& s : spring) {
				for (auto& root : s.SpringDataChain) {
					FTransform currentTransform = FTransform::Identity;

					for (int ChildCount = 0; ChildCount < root.Num(); ++ChildCount) {
						//for (auto& sData : root) {
						auto& sData = root[ChildCount];
						sData.m_length = sData.m_boneAxis.Size();
						int myBoneIndex = RefSkeleton.FindBoneIndex(sData.boneName);

#if	UE_VERSION_OLDER_THAN(5,0,0)
						if (sData.boneIndex >= Output.Pose.GetPose().GetBoneContainer().GetSkeletonToPoseBoneIndexArray().Num()) {
							continue;
						}
#else
						if (Output.Pose.GetPose().GetBoneContainer().GetMeshPoseIndexFromSkeletonPoseIndex(FSkeletonPoseBoneIndex(sData.boneIndex)) == FMeshPoseBoneIndex(INDEX_NONE)) {
							continue;
						}
#endif

						{
							FQuat ParentRotation = FQuat::Identity;
							if (ChildCount == 0) {
								FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(myBoneIndex);

								if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
									continue;
								}

								FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(uu);
								ParentRotation = NewBoneTM.GetRotation();

								currentTransform = NewBoneTM;
							}
							else {
								auto c = RefSkeletonTransform[sData.boneIndex];
								auto t = c * currentTransform;

								ParentRotation = t.GetRotation();
								currentTransform = t;
							}
						}
						FVector v = currentTransform.GetLocation() + sData.m_boneAxis;
						sData.m_currentTail = sData.m_prevTail = ComponentTransform.TransformPosition(v);
					}
				}
			}
		}

		// collider
		colliderGroup.SetNum(meta->VRMColliderMeta.Num());
		for (int i = 0; i < colliderGroup.Num(); ++i) {
			auto& cg = colliderGroup[i];
			const auto& cmeta = meta->VRMColliderMeta[i];

			cg.node = cmeta.bone;
			cg.node_name = *cmeta.boneName;

			cg.colliders.SetNum(cmeta.collider.Num());
			for (int c = 0; c < cg.colliders.Num(); ++c) {
				cg.colliders[c].offset = cmeta.collider[c].offset;
				cg.colliders[c].radius = cmeta.collider[c].radius;
			}

			TArray<VRMSpringCollider> colliders;
		}


		bInit = true;
	}
	void VRMSpringManager::update(const FAnimNode_VrmSpringBone* animNode, float DeltaTime, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
		for (int i = 0; i < spring.Num(); ++i) {
			FTransform c;
			//c = Output.AnimInstanceProxy->GetComponentTransform();
			c = Output.AnimInstanceProxy->GetActorTransform();

			spring[i].Update(animNode, DeltaTime, c, colliderGroup, Output);
		}
	}
	void VRMSpringManager::applyToComponent(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {

		const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
		const auto& RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();


		VRMSpringManager* SpringManager = this;

		for (auto& springRoot : SpringManager->spring) {
			for (auto& sChain : springRoot.SpringDataChain) {
				int BoneChain = 0;

				FTransform CurrentTransForm = FTransform::Identity;
				for (auto& sData : sChain) {


					//FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(sData.boneIndex);
					FCompactPoseBoneIndex uu(sData.boneIndex);

					if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
						continue;
					}

					FTransform NewBoneTM;

					if (BoneChain == 0) {
						NewBoneTM = Output.Pose.GetComponentSpaceTransform(uu);
						NewBoneTM.SetRotation(sData.m_resultQuat);

						CurrentTransForm = NewBoneTM;
					}
					else {

						NewBoneTM = CurrentTransForm;

						auto c = RefSkeletonTransform[sData.boneIndex];
						NewBoneTM = c * NewBoneTM;
						NewBoneTM.SetRotation(sData.m_resultQuat);


						//const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
						//NewBoneTM.SetLocation(ComponentTransform.TransformPosition(sData.m_currentTail));

						CurrentTransForm = NewBoneTM;
					}

					FBoneTransform a(uu, NewBoneTM);

					bool bFirst = true;
					for (auto& t : OutBoneTransforms) {
						if (t.BoneIndex == a.BoneIndex) {
							bFirst = false;
							break;
						}
					}

					if (bFirst) {
						OutBoneTransforms.Add(a);
					}
					BoneChain++;
				}
			}

		}
		OutBoneTransforms.Sort(FCompareBoneTransformIndex());
	}

}

////////////////////////////
namespace VRM1Spring {

	void VRM1SpringManager::reset() {
		for (auto &j : JointStateMap) {
			j.Value.currentTail = j.Value.prevTail = j.Value.initialTail;
		}
	}

	void VRM1SpringManager::init(const UVrmMetaObject* meta, FComponentSpacePoseContext& Output) {

		if (meta == nullptr) return;

		const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
		FTransform ComponentToLocal = ComponentTransform.Inverse();

		vrmMetaObject = meta;
		skeletalMesh = VRMGetSkinnedAsset(Output.AnimInstanceProxy->GetSkelMeshComponent());
		const FReferenceSkeleton& RefSkeleton = VRMGetRefSkeleton(skeletalMesh);
		//const auto& RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();
		const auto& RefSkeletonTransform = RefSkeleton.GetRefBonePose();


		for (auto& s : vrmMetaObject->VRM1SpringBoneMeta.Springs) {
			for (int jointNo = 0; jointNo < s.joints.Num(); jointNo++) {

				auto &j1 = s.joints[jointNo];

				if (j1.boneNo == INDEX_NONE) {
					continue;
				}

				int parentBoneIndex = RefSkeleton.GetParentIndex(j1.boneNo);
				if (parentBoneIndex < 0) continue;
				FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(parentBoneIndex);
				if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
					continue;
				}

				if (RefSkeletonTransform.IsValidIndex(j1.boneNo) == false) {
					continue;
				}

				// VRMC_springBone 1.0 defines consecutive joints[j] and joints[j+1]
				// as a Head/Tail pair: boneAxis points from the Head to its authored
				// Tail in Head-local space, composed across any plain (non-joint)
				// nodes in between. The last joint is Tail-only and is not simulated.
				if (jointNo + 1 >= s.joints.Num()) {
					continue;
				}

				const int32 TailBoneNo = s.joints[jointNo + 1].boneNo;
				FVector TailInHead = FVector::ZeroVector;
				TArray<int32> TailInterBoneNos;
				bool bTailResolved = false;
				if (TailBoneNo != INDEX_NONE && RefSkeletonTransform.IsValidIndex(TailBoneNo)) {
					FVector v = FVector::ZeroVector;
					int32 walk = TailBoneNo;
					while (walk != INDEX_NONE && walk != j1.boneNo && RefSkeletonTransform.IsValidIndex(walk)) {
						v = RefSkeletonTransform[walk].TransformPosition(v);
						if (walk != TailBoneNo) {
							TailInterBoneNos.Insert(walk, 0);
						}
						walk = RefSkeleton.GetParentIndex(walk);
					}
					if (walk == j1.boneNo && v.IsNearlyZero() == false) {
						TailInHead = v;
						bTailResolved = true;
					}
				}

				auto& state = JointStateMap.FindOrAdd(j1.boneNo);

				state.initialLocalMatrix = RefSkeletonTransform[j1.boneNo];
				state.initialLocalRotation = RefSkeletonTransform[j1.boneNo].GetRotation();

				// Anchor = the previous authored joint, if this joint descends from it.
				state.anchorBoneNo = INDEX_NONE;
				state.anchorInterBoneNos.Reset();
				if (jointNo > 0 && s.joints[jointNo - 1].boneNo != INDEX_NONE) {
					const int32 AnchorBoneNo = s.joints[jointNo - 1].boneNo;
					TArray<int32> AnchorInterBoneNos;
					int32 walk = parentBoneIndex;
					bool bAnchorResolved = false;
					while (walk != INDEX_NONE) {
						if (walk == AnchorBoneNo) {
							bAnchorResolved = true;
							break;
						}
						AnchorInterBoneNos.Insert(walk, 0);
						walk = RefSkeleton.GetParentIndex(walk);
					}
					if (bAnchorResolved) {
						state.anchorBoneNo = AnchorBoneNo;
						state.anchorInterBoneNos = AnchorInterBoneNos;
					}
				}

				state.tailBoneNo = INDEX_NONE;
				state.tailInterBoneNos.Reset();
				if (bTailResolved) {
					state.tailBoneNo = TailBoneNo;
					state.tailInterBoneNos = TailInterBoneNos;
					state.boneLength = TailInHead.Length();
					state.boneAxis = TailInHead.GetSafeNormal();
				} else {
					// Tail missing or not a descendant: keep a scaled legacy axis so
					// the joint stays stable instead of guessing an unresolvable chain.
					const FVector FallbackAxis = RefSkeletonTransform[j1.boneNo].GetLocation() * 0.7f;
					state.boneLength = FallbackAxis.Length();
					state.boneAxis = FallbackAxis.GetSafeNormal();
				}

				{
					FCompactPoseBoneIndex u = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(j1.boneNo);
					auto t = Output.Pose.GetComponentSpaceTransform(u);

					const FVector InitialTailLocal = t.GetLocation()
						+ t.GetRotation().RotateVector(state.boneAxis * state.boneLength);
					state.prevTail =
						state.currentTail =
						state.initialTail = ComponentToLocal.InverseTransformPosition(InitialTailLocal);

					state.resultQuat = t.GetRotation();
				}
			}
		}
		bInit = true;
	}

	void VRM1SpringManager::update(const FAnimNode_VrmSpringBone* animNode, float DeltaTime, FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {

		if (skeletalMesh == nullptr) {
			return;
		}
		if (FMath::IsNearlyZero(DeltaTime)) {
			return;
		}

		const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
		//const auto& RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();
		const auto& RefSkeletonTransform = RefSkeleton.GetRefBonePose();

		const FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
		// モデルローカル座標
		// 			FTransform c;
		//c = Output.AnimInstanceProxy->GetComponentTransform();
		//c = Output.AnimInstanceProxy->GetActorTransform();

		const FTransform ComponentToLocal = ComponentTransform.Inverse();

		// 計算した揺れ骨、または揺れ骨の親
		TMap<int, FTransform> calcTransform;

		// 揺れ骨一覧
		TArray<int> springBoneNoList;
		{
			for (auto& s : vrmMetaObject->VRM1SpringBoneMeta.Springs) {
				for (auto& j : s.joints) {
					springBoneNoList.AddUnique(j.boneNo);
				}
			}
		}

		for (auto& s : vrmMetaObject->VRM1SpringBoneMeta.Springs) {
			
			for (int jointNo = 0; jointNo < s.joints.Num(); ++jointNo) {
				FTransform parentTransform = FTransform::Identity;
				FTransform currentTransform = FTransform::Identity;

				auto& j1 = s.joints[jointNo];

				auto* state = JointStateMap.Find(j1.boneNo);
				if (state == nullptr) {
					// Tail-only joints have no state by design. Re-init only when the
					// manager never initialized, or this would re-run every frame.
					if (bInit == false) {
						init(vrmMetaObject, Output);
					}
					continue;
				}

				if (RefSkeletonTransform.IsValidIndex(j1.boneNo) == false) {
					continue;
				}

				int parentBoneIndex = RefSkeleton.GetParentIndex(j1.boneNo);
				if (parentBoneIndex < 0) {
					continue;
				}

				// Anchor to the previous authored joint's simulated transform,
				// composing rest transforms across plain nodes in between
				// (VRMC_springBone 1.0 allows Head to be any ancestor of Tail).
				const FTransform* AnchorTransform = (state->anchorBoneNo != INDEX_NONE)
					? calcTransform.Find(state->anchorBoneNo) : nullptr;
				const FTransform* DirectParentCalc = calcTransform.Find(parentBoneIndex);
				if (AnchorTransform || DirectParentCalc) {
					// 親が揺れ骨。揺れ骨計算結果から参照
					FTransform t = AnchorTransform ? *AnchorTransform : *DirectParentCalc;
					if (AnchorTransform) {
						for (int32 InterBoneNo : state->anchorInterBoneNos) {
							if (RefSkeletonTransform.IsValidIndex(InterBoneNo) == false) {
								continue;
							}
							t = RefSkeletonTransform[InterBoneNo] * t;
						}
					}
					parentTransform = t;
					currentTransform = RefSkeletonTransform[j1.boneNo] * parentTransform;
				} else {
					// 親が揺れ骨ではない。通常骨から参照

					// 親
					{
						FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(parentBoneIndex);
						if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
							continue;
						}
						parentTransform = Output.Pose.GetComponentSpaceTransform(uu);
					}

					{
						// 自分
						FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(j1.boneNo);
						if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
							continue;
						}
						currentTransform = Output.Pose.GetComponentSpaceTransform(uu);
					}
				}

				FQuat m_localRotation = state->initialLocalMatrix.GetRotation();

				const FVector currentTail = ComponentToLocal.TransformPosition(state->currentTail);
				const FVector prevTail = ComponentToLocal.TransformPosition(state->prevTail);

				const FVector inertia = (currentTail - prevTail) * (1.0f - j1.dragForce);
				const FVector stiffness = currentTransform.GetRotation() * state->boneAxis * 1.f * DeltaTime
					* 100.f * j1.stiffness * animNode->stiffnessScale + animNode->stiffnessAdd;

				FVector ue4grav(-j1.gravityDir.X, j1.gravityDir.Z, j1.gravityDir.Y);
				ue4grav = vrmMetaObject->VrmAssetListObject->model_root_transform.Inverse().TransformVector(ue4grav);
				FVector external = ComponentToLocal.TransformVector(ue4grav) * (j1.gravityPower * DeltaTime) * animNode->gravityScale
					+ ComponentToLocal.TransformVector(animNode->gravityAdd) * DeltaTime;


				FVector nextTailTarget = currentTail + inertia + stiffness + external;
				//FVector nextTailTarget = currentTail + stiffness * 10;

				// 長さをboneLengthに強制
				FVector nextTailDirection = (nextTailTarget - currentTransform.GetLocation()).GetSafeNormal();
				FVector nextTailPosition = currentTransform.GetLocation() + nextTailDirection * state->boneLength;


				// vrm <-> vrm collision
				if (animNode->bIgnoreVRMCollision == false) {

					// このSpringが参照するコライダのインデックス
					TArray<int> checkcolliderIndexArray;
					for (auto colg : s.colliderGroups) {
						checkcolliderIndexArray.Append(vrmMetaObject->VRM1SpringBoneMeta.ColliderGroups[colg].colliders);
					}

					// 全てのコライダ
					auto& AllColliderArray = vrmMetaObject->VRM1SpringBoneMeta.Colliders;

					for (auto colNo : checkcolliderIndexArray){
						if (colNo >= AllColliderArray.Num()) {
							continue;
						}
						const auto& collider = AllColliderArray[colNo];

						FTransform collisionBoneTrans;
						{
							int boneNo = RefSkeleton.FindBoneIndex(*collider.boneName);
							if (boneNo < 0) {
								continue;
							}

							FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(boneNo);
							//FCompactPoseBoneIndex uu(ii);
							if (uu == INDEX_NONE) {
								continue;
							}
							collisionBoneTrans = Output.Pose.GetComponentSpaceTransform(uu);
						}

						auto offs = collider.offset;
						offs.Set(offs.X, -offs.Z, offs.Y);
						offs *= 100;
						//offs = collisionBoneTrans.TransformVector(offs);

						auto tail = collider.tail;
						tail.Set(tail.X, -tail.Z, tail.Y);
						tail *= 100;
						//tail = collisionBoneTrans.TransformVector(tail);

						float r = (j1.hitRadius + collider.radius) * 100.f;

						if (collider.shapeType == TEXT("sphere")) {
							FVector v = collisionBoneTrans.TransformPosition(offs);

							if ((v - nextTailPosition).SizeSquared() > r * r) {
								continue;
							}
							// ヒット。Colliderの半径方向に押し出す
							auto normal = (nextTailPosition - v).GetSafeNormal();
							auto posFromCollider = v + normal * r;
							// 長さをboneLengthに強制
							nextTailPosition = currentTransform.GetLocation() + (posFromCollider - currentTransform.GetLocation()).GetSafeNormal() * state->boneLength;
							nextTailDirection = (posFromCollider - currentTransform.GetLocation()).GetSafeNormal();
						}
						else {
	
							FTransform t1 = collisionBoneTrans;
							auto v1 = t1.TransformPosition(offs);

							FTransform t2 = collisionBoneTrans;
							auto v2 = t2.TransformPosition(tail);

							FVector nearestPoint = FMath::ClosestPointOnSegment(nextTailPosition, v1, v2);

							float dif = (nearestPoint - nextTailPosition).SizeSquared();
							if (dif > r * r) {
								continue;
							}

							auto normal = (nextTailPosition - nearestPoint).GetSafeNormal();

							auto posFromCollider = nearestPoint + normal * r;
							// 長さをboneLengthに強制
							nextTailPosition = currentTransform.GetLocation() + (posFromCollider - currentTransform.GetLocation()).GetSafeNormal() * state->boneLength;
							nextTailDirection = (posFromCollider - currentTransform.GetLocation()).GetSafeNormal();
						}
					}
				}



				state->prevTail = ComponentToLocal.InverseTransformPosition(currentTail);
				state->currentTail = ComponentToLocal.InverseTransformPosition(nextTailPosition);

				{
					FVector from = currentTransform.TransformVector(state->boneAxis).GetSafeNormal();
					FVector to = nextTailDirection;

					state->resultQuat = FQuat::FindBetween(from, to) * parentTransform.GetRotation() * state->initialLocalMatrix.GetRotation();
				}

				// initial rot
				{
					//state->resultQuat = currentTransform.GetRotation();
				}
				{
					//state->resultQuat = (state->initialLocalMatrix * parentTransform).GetRotation();
				}
				{
					//state->resultQuat = parentTransform.GetRotation() * state->initialLocalRotation;
				}

				currentTransform.SetRotation(state->resultQuat);

				// 揺れ骨計算結果を保持。これの子の揺れ骨のため。
				calcTransform.Add(j1.boneNo, currentTransform);
			}
		}
	}

	static void AddUniqueBoneTransform(TArray<FBoneTransform>& OutBoneTransforms, const FBoneTransform& BoneTransform) {
		for (auto& t : OutBoneTransforms) {
			if (t.BoneIndex == BoneTransform.BoneIndex) {
				return;
			}
		}
		OutBoneTransforms.Add(BoneTransform);
	}

	void VRM1SpringManager::applyToComponent(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {

		const auto RefSkeleton = Output.AnimInstanceProxy->GetSkeleton()->GetReferenceSkeleton();
		const auto& RefSkeletonTransform = Output.Pose.GetPose().GetBoneContainer().GetRefPoseArray();

		// 計算した揺れ骨、または揺れ骨の親
		TMap<int, FTransform> calcTransform;

		// 揺れ骨一覧
		TArray<int> springBoneNoList;
		{
			for (auto& s : vrmMetaObject->VRM1SpringBoneMeta.Springs) {
				for (auto& j : s.joints) {
					springBoneNoList.AddUnique(j.boneNo);
				}
			}
		}

		for (auto& s : vrmMetaObject->VRM1SpringBoneMeta.Springs) {

			FTransform CurrentTransForm;

			for (int jointNo = 0; jointNo < s.joints.Num(); ++jointNo) {
				auto& j = s.joints[jointNo];

				auto* state = JointStateMap.Find(j.boneNo);
				if (state == nullptr) continue;

				FCompactPoseBoneIndex uu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(j.boneNo);

				if (Output.Pose.GetPose().IsValidIndex(uu) == false) {
					continue;
				}

				FTransform NewBoneTM;

				int parentBoneIndex = RefSkeleton.GetParentIndex(j.boneNo);
				const FTransform* AnchorTransform = (state->anchorBoneNo != INDEX_NONE)
					? calcTransform.Find(state->anchorBoneNo) : nullptr;
				const FTransform* DirectParentCalc = calcTransform.Find(parentBoneIndex);
				if (AnchorTransform || DirectParentCalc) {
					// 親は揺れ骨。計算結果から参照する
					FTransform t = AnchorTransform ? *AnchorTransform : *DirectParentCalc;
					if (AnchorTransform) {
						// Plain nodes between the previous joint and this joint follow
						// rigidly, like scene-graph propagation in the source model.
						for (int32 InterBoneNo : state->anchorInterBoneNos) {
							if (RefSkeletonTransform.IsValidIndex(InterBoneNo) == false) {
								continue;
							}
							t = RefSkeletonTransform[InterBoneNo] * t;
							FCompactPoseBoneIndex iu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(InterBoneNo);
							if (Output.Pose.GetPose().IsValidIndex(iu)) {
								AddUniqueBoneTransform(OutBoneTransforms, FBoneTransform(iu, t));
							}
						}
					}
					NewBoneTM = RefSkeletonTransform[j.boneNo] * t;
					NewBoneTM.SetRotation(state->resultQuat);
					CurrentTransForm = NewBoneTM;
				}
				else {
					// 親は揺れ骨でない。現在値に差分を適用
					NewBoneTM = Output.Pose.GetComponentSpaceTransform(uu);
					NewBoneTM.SetRotation(state->resultQuat);
					CurrentTransForm = NewBoneTM;
				}

				AddUniqueBoneTransform(OutBoneTransforms, FBoneTransform(uu, NewBoneTM));

				calcTransform.Add(j.boneNo, CurrentTransForm);

				// The authored chain tail (and plain nodes before it) is Tail-only:
				// not simulated, so move it rigidly with this joint to keep the
				// skinned chain connected.
				if (state->tailBoneNo != INDEX_NONE && JointStateMap.Contains(state->tailBoneNo) == false) {
					FTransform t = CurrentTransForm;
					TArray<int32> FollowBoneNos = state->tailInterBoneNos;
					FollowBoneNos.Add(state->tailBoneNo);
					for (int32 FollowBoneNo : FollowBoneNos) {
						if (RefSkeletonTransform.IsValidIndex(FollowBoneNo) == false) {
							break;
						}
						FCompactPoseBoneIndex fu = Output.Pose.GetPose().GetBoneContainer().GetCompactPoseIndexFromSkeletonIndex(FollowBoneNo);
						if (Output.Pose.GetPose().IsValidIndex(fu) == false) {
							break;
						}
						t = RefSkeletonTransform[FollowBoneNo] * t;
						AddUniqueBoneTransform(OutBoneTransforms, FBoneTransform(fu, t));
					}
				}
				// update rotation
				//FVector to = (state->currentTail * (node.parent.worldMatrix * state->initialLocalMatrix).inverse).normalized;
				//node.rotation = initialLocalRotation * Quaternion.fromToQuaternion(boneAxis, to);
			}
		}
		OutBoneTransforms.Sort(FCompareBoneTransformIndex());

	}
} //spring1
