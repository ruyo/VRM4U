// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.


#include "VrmModelActor.h"


// Sets default values
AVrmModelActor::AVrmModelActor(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AVrmModelActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVrmModelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

