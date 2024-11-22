// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Units/RigUnit.h"
#include "ControlRigDefines.h" 

#include "RigUnit_VrmDynamicHierarchy.generated.h"

/**
 * add morph and curve
 */
USTRUCT(meta=(DisplayName="VRM4U AddCurveFromMesh", Keywords="AddCurve,Spawn", Varying))
struct FRigUnit_VRMAddCurveFromMesh : public FRigUnitMutable
{
	GENERATED_BODY()

	FRigUnit_VRMAddCurveFromMesh()
	{
		//Prefix = TEXT("c_");
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Input))
	FString Prefix = TEXT("");

	UPROPERTY(meta = (Input))
	FString Suffix = TEXT("_c");

	UPROPERTY(meta = (Input))
	bool bIncludeMorphs = true;

	UPROPERTY(meta = (Input))
	bool bIncludeCurves = true;

	UPROPERTY(meta = (Output))
	TArray<FRigElementKey> Items_Morph;

	UPROPERTY(meta = (Output))
	TArray<FRigElementKey> Items_Curve;
};

