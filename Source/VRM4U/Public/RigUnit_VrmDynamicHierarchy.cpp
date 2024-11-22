// Copyright Epic Games, Inc. All Rights Reserved.


#include "RigUnit_VrmDynamicHierarchy.h"
#include "Engine/SkeletalMesh.h"
#include "Rigs/RigHierarchyController.h"
#include "Units/RigUnitContext.h"
#include "ControlRig.h"
#include "Components/SkeletalMeshComponent.h"
//#include "Units/Execution/RigUnit_PrepareForExecution.h"

//#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_DynamicHierarchy)


FRigUnit_VRMAddCurveFromMesh_Execute()
{
	FString ErrorMessage;

	Items_Curve.Reset();
	Items_Morph.Reset();

	const USkeletalMeshComponent* skc = ExecuteContext.UnitContext.DataSourceRegistry->RequestSource<USkeletalMeshComponent>(UControlRig::OwnerComponent);
	if (skc == nullptr) return;

	URigHierarchyController* Controller = ExecuteContext.Hierarchy->GetController(true);
	if (Controller == nullptr) return;


	FRigControlSettings ControlSettings_root;
	ControlSettings_root.ControlType = ERigControlType::Float

	FRigControlSettings ControlSettings;

	ControlSettings.ControlType = ERigControlType::Float;
	ControlSettings.PrimaryAxis = ERigControlAxis::X;

	ControlSettings.SetupLimitArrayForType(false, false, false);
	//ControlSettings.LimitEnabled[0] = Limit;
	ControlSettings.MinimumValue = FRigControlValue::Make<float>(0);
	ControlSettings.MaximumValue = FRigControlValue::Make<float>(1);
	//ControlSettings.bDrawLimits = bDrawLimits;


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
		}
	}
}
