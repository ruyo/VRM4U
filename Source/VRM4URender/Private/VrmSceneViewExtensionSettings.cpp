// Fill out your copyright notice in the Description page of Project Settings.


#include "VrmSceneViewExtensionSettings.h"
#include "VRM4U_RenderSubsystem.h"
#include "Engine/World.h"

// Sets default values
AVrmSceneViewExtensionSettings::AVrmSceneViewExtensionSettings()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AVrmSceneViewExtensionSettings::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVrmSceneViewExtensionSettings::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AVrmSceneViewExtensionSettings::OnConstruction(const FTransform& Transform) {
	Super::OnConstruction(Transform);
}

