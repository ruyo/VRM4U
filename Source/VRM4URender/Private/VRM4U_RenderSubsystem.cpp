// Fill out your copyright notice in the Description page of Project Settings.


#include "VRM4U_RenderSubsystem.h"
#include "VrmExtensionRimFilterData.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResources.h"
#include "RenderGraphUtils.h"
#include "EngineModule.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "Engine/SkeletalMesh.h"
#include "RendererInterface.h"
#include "ScreenPass.h"
#include "Runtime/Renderer/Private/SceneRendering.h"
#include "SceneRenderTargetParameters.h"
#include "Slate/SceneViewport.h"
#include "ScreenRendering.h"
#if UE_VERSION_OLDER_THAN(5,4,0)
#include "Runtime/Renderer/Private/SceneTextures.h"
#endif


#if WITH_EDITOR
#include "LevelEditor.h"
#endif

void UVRM4U_RenderSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);

	// generate
	SceneViewExtension = FSceneViewExtensions::NewExtension<FVrmSceneViewExtension>();
}

void UVRM4U_RenderSubsystem::Deinitialize() {

	{
		FScopeLock lock(&cs_rim);
		RimFilterData.Empty();
	}
	Super::Deinitialize();
}

void UVRM4U_RenderSubsystem::OnResolvedSceneColor_RenderThread(FRDGBuilder& GraphBuilder, const FSceneTextures& SceneTextures) {
}


#if WITH_EDITOR
void UVRM4U_RenderSubsystem::OnMapChange(UWorld* World, EMapChangeType ChangeType) {
	FVRM4URenderModule* m = FModuleManager::GetModulePtr<FVRM4URenderModule>("VRM4URender");
	if (m != nullptr) {
		m->OnMapChange(World, ChangeType);
	}
}

void UVRM4U_RenderSubsystem::OnPIEEvent(bool bPIEBegin, bool bPIEEnd) {
	FVRM4URenderModule* m = FModuleManager::GetModulePtr<FVRM4URenderModule>("VRM4URender");
	if (m != nullptr) {
		m->OnPIEEvent(bPIEBegin, bPIEEnd);
	}
}
#endif


void UVRM4U_RenderSubsystem::RenderPre(FRDGBuilder& GraphBuilder) {
}
void UVRM4U_RenderSubsystem::RenderPost(FRDGBuilder& GraphBuilder) {
}

void UVRM4U_RenderSubsystem::AddCaptureTexture(UTextureRenderTarget2D* Texture, EVRM4U_CaptureSource CaptureSource) {
	if (Texture == nullptr) return;

	FVRM4URenderModule* m = FModuleManager::GetModulePtr<FVRM4URenderModule>("VRM4URender");
	if (m != nullptr){
		m->CaptureList.FindOrAdd(Texture) = CaptureSource;
	}


#if WITH_EDITOR
	if (HandleTearDown.IsValid() == false){
		if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
		{
			FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");

				
			HandleTearDown = LevelEditor.OnMapChanged().AddUObject(this, &UVRM4U_RenderSubsystem::OnMapChange);
					/*
				HandleTearDown = LevelEditor.OnMapChanged().AddLambda([&](UWorld* World, EMapChangeType ChangeType)
				{
					if (ChangeType == EMapChangeType::TearDownWorld)
					{
						CaptureList.Empty();
					}
				});
				*/
		}
	}

	if (bInitPIE == false) {
		bInitPIE = true;
		FEditorDelegates::BeginPIE.AddLambda([&](const bool bIsSimulating) {
			this->OnPIEEvent(true, false);
			});
		FEditorDelegates::EndPIE.AddLambda([&](const bool bIsSimulating) {
			this->OnPIEEvent(false, true);
			});
	}
#endif
}

void UVRM4U_RenderSubsystem::RemoveCaptureTexture(UTextureRenderTarget2D* Texture) {
	FVRM4URenderModule* m = FModuleManager::GetModulePtr<FVRM4URenderModule>("VRM4URender");
	if (m != nullptr) {
		m->CaptureList.Remove(Texture);
	}
}

void UVRM4U_RenderSubsystem::RemoveAllCaptureTexture() {
	FVRM4URenderModule* m = FModuleManager::GetModulePtr<FVRM4URenderModule>("VRM4URender");
	if (m != nullptr) {
		m->CaptureList.Empty();
	}
}


void UVRM4U_RenderSubsystem::ResetSceneTextureExtentHistory() {
	::ResetSceneTextureExtentHistory();
}

void UVRM4U_RenderSubsystem::SetViewExtension(bool bEnable) {
}

void UVRM4U_RenderSubsystem::AddRimFilterData(class UVrmExtensionRimFilterData* FilterData) {
	FScopeLock lock(&cs_rim);
	RimFilterData.AddUnique(FilterData);
}

void UVRM4U_RenderSubsystem::RemoveRimFilterData(class UVrmExtensionRimFilterData* FilterData) {
	FScopeLock lock(&cs_rim);
	RimFilterData.Remove(FilterData);
}

void UVRM4U_RenderSubsystem::RemoveRimFilterDataByPriority(int Priority) {
	FScopeLock lock(&cs_rim);
	if (Priority < 0) {
		RimFilterData.Empty();
		return;
	}
	RimFilterData.RemoveAll([&Priority](const TWeakObjectPtr<UVrmExtensionRimFilterData>& Item) {
		if (Item.IsValid() == false) return true;
		return Item->Priority == Priority;
		});
}

TArray<UVrmExtensionRimFilterData::FFilterData> UVRM4U_RenderSubsystem::GenerateFilterData() {
	FScopeLock lock(&cs_rim);

	RimFilterData.RemoveAll([](const TWeakObjectPtr<UVrmExtensionRimFilterData>& Item) {
		return !Item.IsValid();
		});

	RimFilterData.Sort([](const TWeakObjectPtr<UVrmExtensionRimFilterData>& A, const TWeakObjectPtr<UVrmExtensionRimFilterData>& B)
		{
			return A->Priority < B->Priority;
		});

	TArray<UVrmExtensionRimFilterData::FFilterData> f;
	for (auto& a : RimFilterData) {
		auto d = a->GetFilterData();
		if (d.bUseFilter == false) continue;
		f.Add(MoveTemp(d));
	}
	return MoveTemp(f);
}


