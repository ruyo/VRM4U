// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmCineCameraActor.h"
#include "VrmCineCameraComponent.h"
#include "Misc/EngineVersionComparison.h"

AVrmCineCameraActor::AVrmCineCameraActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
		.SetDefaultSubobjectClass<UVrmCineCameraComponent>(TEXT("CameraComponent"))
	)
{
}


UVrmCineCameraComponent* AVrmCineCameraActor::GetVrmCineCameraComponent() const {
	return Cast<UVrmCineCameraComponent>(GetCineCameraComponent());
}
