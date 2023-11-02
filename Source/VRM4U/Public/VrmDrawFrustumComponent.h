// Copyright Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/PrimitiveComponent.h"
#include "Components/DrawFrustumComponent.h"
#include "VrmDrawFrustumComponent.generated.h"

class FPrimitiveSceneProxy;

/**
 *	Utility component for drawing a view frustum. Origin is at the component location, frustum points down position X axis.
 */ 

UCLASS(collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UVrmDrawFrustumComponent : public UDrawFrustumComponent
{
	GENERATED_UCLASS_BODY()

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DrawFrustumComponent)
	FVector2D OffCenterProjectionOffset;
};



