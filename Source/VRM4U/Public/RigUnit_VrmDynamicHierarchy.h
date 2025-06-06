// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Units/RigUnit.h"
#include "ControlRigDefines.h" 

#include "RigUnit_VrmDynamicHierarchy.generated.h"


/**
 * get curve list
 */
USTRUCT(meta = (DisplayName = "VRM4U GetCurveNameFromMesh", Keywords = "VRM4U", Varying))
struct FRigUnit_VRMGetCurveNameFromMesh : public FRigUnitMutable
{
	GENERATED_BODY()

	FRigUnit_VRMGetCurveNameFromMesh()
	{
	}

	RIGVM_METHOD()
		virtual void Execute() override;

	UPROPERTY(meta = (Output))
	TArray<FName> Items_Morph;

	UPROPERTY(meta = (Output))
	TArray<FName> Items_Curve;
};


/**
 * add morph and curve
 */
USTRUCT(meta=(DisplayName="VRM4U AddCurveFromMesh", Keywords="AddCurve,Spawn", Varying))
struct FRigUnit_VRMAddCurveFromMesh : public FRigUnitMutable
{
	GENERATED_BODY()

	FRigUnit_VRMAddCurveFromMesh()
	{
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


/**
 * generate contorl to bone table
 */
USTRUCT(meta = (DisplayName = "VRM4U GenerateTable", Keywords = "Table", Varying))
struct FRigUnit_VRMGenerateBoneToControlTable: public FRigUnitMutable
{
	GENERATED_BODY()

	FRigUnit_VRMGenerateBoneToControlTable()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	UPROPERTY(meta = (Output))
	TArray<FName> Items_MannequinBone;

	UPROPERTY(meta = (Output))
	TArray<FName> Items_VRMBone;
};

/**
 * init controller transform
 */
USTRUCT(meta = (DisplayName = "VRM4U InitController Transform", Keywords = "Table", Varying))
struct FRigUnit_VRMInitControllerTransform: public FRigUnitMutable
{
	GENERATED_BODY()

	FRigUnit_VRMInitControllerTransform()
	{
	}

	RIGVM_METHOD()
	virtual void Execute() override;
};


