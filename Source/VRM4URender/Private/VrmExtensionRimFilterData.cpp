// Fill out your copyright notice in the Description page of Project Settings.


#include "VrmExtensionRimFilterData.h"
#include "VRM4U_RenderSubsystem.h"
#include "Engine/World.h"

// Sets default values
UVrmExtensionRimFilterData::UVrmExtensionRimFilterData(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = false;

}


/*
void UVrmExtensionRimFilterData::BeginDestroy() {
	UVRM4U_RenderSubsystem* s = GEngine->GetEngineSubsystem<UVRM4U_RenderSubsystem>();
	if (s == nullptr) return;

	s->RemoveRimFilterData(this);

	Super::BeginDestroy();
}
*/


void UVrmExtensionRimFilterData::PostInitProperties() {
	Super::PostInitProperties();

	if (GEngine == nullptr) return;

	if (!HasAnyFlags(RF_ClassDefaultObject)) {
		// インスタンスの場合
		UVRM4U_RenderSubsystem* s = GEngine->GetEngineSubsystem<UVRM4U_RenderSubsystem>();
		if (s == nullptr) return;

		s->AddRimFilterData(this);
	}
}

// Called every frame
//void UVrmExtensionRimFilterData::Tick(float DeltaTime)
//{
//	Super::Tick(DeltaTime);
//}

/*
void UVrmExtensionRimFilterData::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);

	//UVRM4U_RenderSubsystem* s = UWorld::GetSubsystem<UVRM4U_RenderSubsystem>(GetWorld());
	UVRM4U_RenderSubsystem* s = GEngine->GetEngineSubsystem<UVRM4U_RenderSubsystem>();
	if (s == nullptr) return;

	s->bUsePostRenderBasePass = bUseExtension;

}
*/

