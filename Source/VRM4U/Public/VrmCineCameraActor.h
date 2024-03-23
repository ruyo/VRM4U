// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"

#include "VrmCineCameraActor.generated.h"


UCLASS(ClassGroup = Common, hideCategories = (Input, Rendering, AutoPlayerActivation), showcategories = ("Input|MouseInput", "Input|TouchInput"), Blueprintable)
class VRM4U_API AVrmCineCameraActor : public ACineCameraActor
{
	GENERATED_BODY()

public:
	// Ctor
	AVrmCineCameraActor(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Camera")
		UVrmCineCameraComponent* GetVrmCineCameraComponent() const;

public:

};
