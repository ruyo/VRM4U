// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UObject/NoExportTypes.h"

#include "Engine/GameViewportClient.h"

//#if WITH_EDITOR
//#include "UnrealEd.h"
//#include "AssetTypeActions_Base.h"
//#endif

#include "VrmGameViewportClient.generated.h"


UCLASS()
class VRM4U_API UVrmGameViewportClient : public UGameViewportClient {

	GENERATED_UCLASS_BODY()
public:

	virtual void Draw(FViewport* Viewport, FCanvas* SceneCanvas) override;
};



