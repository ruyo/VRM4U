// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VrmSceneViewExtensionSettings.generated.h"

UCLASS()
class VRM4URENDER_API AVrmSceneViewExtensionSettings : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVrmSceneViewExtensionSettings();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	bool bUseExtension = false;


	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;
};
