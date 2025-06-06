// Copyright Epic Games, Inc. All Rights Reserved.


#include "RigUnit_VrmDynamicHierarchy.h"
#include "Engine/SkeletalMesh.h"
#include "Rigs/RigHierarchyController.h"
#include "Units/RigUnitContext.h"
#include "ControlRig.h"
#include "Components/SkeletalMeshComponent.h"

#include "VrmUtil.h"
#include "VrmAssetListObject.h"
#include "VrmMetaObject.h"

//#include "Units/Execution/RigUnit_PrepareForExecution.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_DynamicHierarchy)

FRigUnit_VRMInitControllerTransform_Execute() {

	const USkeletalMeshComponent* skc = ExecuteContext.UnitContext.DataSourceRegistry->RequestSource<USkeletalMeshComponent>(UControlRig::OwnerComponent);
	if (skc == nullptr) return;

	auto* sk = VRMGetSkeletalMeshAsset(skc);
	if (sk == nullptr) return;

	auto* k = sk->GetSkeleton();
	if (k == nullptr) return;


	URigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true);
	if (Controller == nullptr) return;

	UVrmAssetListObject* assetList = VRMUtil::GetAssetListObject(sk);

	if (assetList == nullptr) return;
	//Controller->

	TArray<FRigControlElement*> controlList = ExecuteContext.Hierarchy->GetControls();
	TArray<FRigNullElement*> nullList = ExecuteContext.Hierarchy->GetNulls();
	//contollerList[0]->GetNameString();

	//ExecuteContext.Hierarchy->SetInitialGlobalTransform
	//Controller


	TMap<FRigElementKey, FTransform> elemInitTransformList;
	for (auto& table : VRMUtil::table_ue4_vrm) {

		auto* elem = nullList.FindByPredicate(
			[&table](FRigNullElement* e) {
#if	UE_VERSION_OLDER_THAN(5,4,0)
				if (e->GetNameString().Compare(table.BoneUE4 + "_s") == 0) {
					return true;
				}
#else
				if (e->GetName().Compare(table.BoneUE4 + "_s") == 0) {
					return true;
				}
#endif
				return false;
			});

		if (elem == nullptr) continue;

		FRigElementKey bone;
		{
			const auto& assetBoneTable = assetList->VrmMetaObject->humanoidBoneTable;

			auto* str = assetBoneTable.Find(table.BoneVRM);
			if (str == nullptr) continue;

			bone.Type = ERigElementType::Bone;
			bone.Name = *(*str);
		}


		FRigElementKey a;
		a.Name = *(table.BoneUE4 + "_s");
		a.Type = ERigElementType::Null;


		auto index = k->GetReferenceSkeleton().FindBoneIndex(bone.Name);
		auto refPose = k->GetReferenceSkeleton().GetRefBonePose();

		elemInitTransformList.Add(a, refPose[index]);
	}

	for (auto& c : controlList) {
		//ExecuteContext.Hierarchy->SetInitialLocalTransform(c->GetKey(), FTransform::Identity);
		//ExecuteContext.Hierarchy->SetLocalTransform(c->GetKey(), FTransform::Identity);
	}
	for (auto& c : nullList) {
		auto *f = elemInitTransformList.Find(c->GetKey());
		if (f) {
			ExecuteContext.Hierarchy->SetInitialGlobalTransform(c->GetKey(), *f);
			ExecuteContext.Hierarchy->SetLocalTransform(c->GetKey(), *f);
		} else {
			ExecuteContext.Hierarchy->SetInitialLocalTransform(c->GetKey(), FTransform::Identity);
			ExecuteContext.Hierarchy->SetLocalTransform(c->GetKey(), FTransform::Identity);
		}
	}

}


FRigUnit_VRMGenerateBoneToControlTable_Execute()
{
	Items_MannequinBone.Empty();
	//Items_MannequinControl.Empty();
	Items_VRMBone.Empty();

	const USkeletalMeshComponent* skc = ExecuteContext.UnitContext.DataSourceRegistry->RequestSource<USkeletalMeshComponent>(UControlRig::OwnerComponent);
	if (skc == nullptr) return;

	auto* sk = VRMGetSkeletalMeshAsset(skc);
	if (sk == nullptr) return;

	auto* k = sk->GetSkeleton();
	if (k == nullptr) return;


	URigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true);
	if (Controller == nullptr) return;

	UVrmAssetListObject *assetList = VRMUtil::GetAssetListObject(sk);

	if (assetList == nullptr) return;
	//Controller->

	TArray<FRigControlElement*> controllerList = ExecuteContext.Hierarchy->GetControls();
	//contollerList[0]->GetNameString();

	//ExecuteContext.Hierarchy->SetInitialGlobalTransform
	//Controller


	for (auto& table : VRMUtil::table_ue4_vrm) {

		auto* elem = controllerList.FindByPredicate(
			[&table](FRigControlElement* e) {
#if	UE_VERSION_OLDER_THAN(5,4,0)
				if (e->GetNameString().Compare(table.BoneUE4 + "_c") == 0) {
					return true;
				}
#else
				if (e->GetName().Compare(table.BoneUE4 + "_c") == 0) {
					return true;
				}
#endif
				return false;
			});

		if (elem == nullptr) continue;

		FRigElementKey bone;
		{
			const auto& assetBoneTable = assetList->VrmMetaObject->humanoidBoneTable;

			auto* str = assetBoneTable.Find(table.BoneVRM);
			if (str == nullptr) continue;

			bone.Type = ERigElementType::Bone;
			bone.Name = *(*str);

			Items_VRMBone.Add(*(*str));
		}


		{
			FRigElementKey a;
			a.Name = *(table.BoneUE4);
			a.Type = ERigElementType::Control;
			Items_MannequinBone.Add(*(table.BoneUE4));
		}
		{
			FRigElementKey a;
			a.Name = *(table.BoneUE4 + "_c");
			a.Type = ERigElementType::Control;
			//Items_MannequinControl.Add(a);
		}
		//Items_VRMBone.Add(bone);
	}
}

FRigUnit_VRMGetCurveNameFromMesh_Execute() {
	FString ErrorMessage;

	Items_Curve.Reset();
	Items_Morph.Reset();

	const USkeletalMeshComponent* skc = ExecuteContext.UnitContext.DataSourceRegistry->RequestSource<USkeletalMeshComponent>(UControlRig::OwnerComponent);
	if (skc == nullptr) return;

	{
		auto* sk = skc->GetSkinnedAsset();
		auto* k = sk->GetSkeleton();
		auto morph = sk->GetMorphTargets();

		const FRigControlValue Value = FRigControlValue::Make<float>(0);
		const FTransform ShapeTransform;
		const FTransform OffsetTransform;

		for (auto& a : morph) {
			Items_Morph.Add(a.GetFName());
		}

#if	UE_VERSION_OLDER_THAN(5,3,0)
		const FSmartNameMapping* CurveMapping = k->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
		if (CurveMapping) {
			TArray<FName> CurveNames;
			CurveMapping->FillNameArray(CurveNames);
			for (const FName& CurveName : CurveNames) {
				auto* meta = CurveMapping->GetCurveMetaData(CurveName);
				if (meta->Type.bMorphtarget == true) {
					continue;
				}
				Items_Curve.Add(CurveName);
			}
		}
#else
		k->ForEachCurveMetaData([&](const FName& InCurveName, const FCurveMetaData& InMetaData)
			{
				if (InMetaData.Type.bMorphtarget == true) {
					return;
				}
				Items_Curve.Add(InCurveName);
			});
#endif
	}

}

FRigUnit_VRMAddCurveFromMesh_Execute()
{
	FString ErrorMessage;

	Items_Curve.Reset();
	Items_Morph.Reset();

	const USkeletalMeshComponent* skc = ExecuteContext.UnitContext.DataSourceRegistry->RequestSource<USkeletalMeshComponent>(UControlRig::OwnerComponent);
	if (skc == nullptr) return;

	URigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true);
	if (Controller == nullptr) return;

	FRigControlSettings ControlSettings;

	ControlSettings.ControlType = ERigControlType::Float;
	ControlSettings.PrimaryAxis = ERigControlAxis::X;

	ControlSettings.MinimumValue = FRigControlValue::Make<float>(0);
	ControlSettings.MaximumValue = FRigControlValue::Make<float>(1);

	ControlSettings.SetupLimitArrayForType(true, true, true);
	ControlSettings.SetVisible(false);

	FRigHierarchyControllerInstructionBracket InstructionBracket(Controller, ExecuteContext.GetInstructionIndex());

	{
		auto *sk = skc->GetSkinnedAsset();
		auto* k = sk->GetSkeleton();
		auto morph = sk->GetMorphTargets();

		const FRigControlValue Value = FRigControlValue::Make<float>(0);
		const FTransform ShapeTransform;
		const FTransform OffsetTransform;

		if (bIncludeMorphs) {
			auto root = Controller->AddNull(TEXT("VRM4U_Root_Morph"), FRigElementKey(), OffsetTransform);
			for (auto& a : morph) {
				FName n = *(Prefix + a.GetFName().ToString() + Suffix);
				Items_Morph.Add(Controller->AddControl(n, root, ControlSettings, Value, OffsetTransform, ShapeTransform, false, false));
			}
		}

		if (bIncludeCurves) {
			auto root = Controller->AddNull(TEXT("VRM4U_Root_Curve"), FRigElementKey(), OffsetTransform);

#if	UE_VERSION_OLDER_THAN(5,3,0)
			const FSmartNameMapping* CurveMapping = k->GetSmartNameContainer(USkeleton::AnimCurveMappingName);
			if (CurveMapping) {
				TArray<FName> CurveNames;
				CurveMapping->FillNameArray(CurveNames);
				for (const FName& CurveName : CurveNames) {
					auto* meta = CurveMapping->GetCurveMetaData(CurveName);
					if (meta->Type.bMorphtarget == true) {
						continue;
					}
					FName n = *(Prefix + CurveName.ToString() + Suffix);
					Items_Curve.Add(Controller->AddControl(n, root, ControlSettings, Value, OffsetTransform, ShapeTransform, false, false));
				}
			}
#else
			k->ForEachCurveMetaData([&](const FName& InCurveName, const FCurveMetaData& InMetaData)
				{
					if (InMetaData.Type.bMorphtarget == true) {
						return;
					}
					FName n = *(Prefix + InCurveName.ToString() + Suffix);
					Items_Curve.Add(Controller->AddControl(n, root, ControlSettings, Value, OffsetTransform, ShapeTransform, false, false));
				});
#endif

		}
	}
}
