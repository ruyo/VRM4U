// Fill out your copyright notice in the Description page of Project Settings.


#include "VRM4U_RenderSubsystem.h"



void UVRM4U_RenderSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);


	//GEngine->GetPreRenderDelegateEx().AddUObject(this, &UVRM4U_RenderSubsystem::RenderPre);
	//GEngine->GetPostRenderDelegateEx().AddUObject(this, &UVRM4U_RenderSubsystem::RenderPost);

}

void UVRM4U_RenderSubsystem::RenderPre(FRDGBuilder& GraphBuilder) {

}
void UVRM4U_RenderSubsystem::RenderPost(FRDGBuilder& GraphBuilder) {

}
