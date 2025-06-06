// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmEditorEventComponent.h"
#include "Misc/EngineVersionComparison.h"

#if WITH_EDITOR
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "Engine/Selection.h"

//#include "MovieSceneTrackEditor.h"
#if	UE_VERSION_OLDER_THAN(4,26,0)
#elif UE_VERSION_OLDER_THAN(4,27,0)
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequenceEditor/Private/LevelSequenceEditorBlueprintLibrary.h"
#elif UE_VERSION_OLDER_THAN(5,0,0)
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequenceEditor/Public/LevelSequenceEditorBlueprintLibrary.h"
#elif UE_VERSION_OLDER_THAN(5,3,0)
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequenceEditor/Public/LevelSequenceEditorBlueprintLibrary.h"
#else
#include "ILevelSequenceEditorToolkit.h"
#include "LevelSequenceEditorBlueprintLibrary.h"
#endif

#if UE_VERSION_OLDER_THAN(5,5,0)
#else
#include "MovieSceneSequencePlayer.h"
#endif


#include "LevelSequence.h"
#include "ISequencer.h"
#endif




UVrmEditorEventComponent::UVrmEditorEventComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

void UVrmEditorEventComponent::OnRegister() {
	Super::OnRegister();
}
void UVrmEditorEventComponent::OnUnregister() {
	Super::OnUnregister();
}

void UVrmEditorEventComponent::OnSelectionChangeFunc(UObject *obj) {
#if WITH_EDITOR
	if (GEditor == nullptr) return;
	bool bFound = false;
	USelection* Selection = Cast<USelection>(obj);
	if (Selection == GEditor->GetSelectedComponents() || Selection == GEditor->GetSelectedActors()){
		for (int32 Idx = 0; Idx < Selection->Num(); Idx++)
		{
			const auto *a = Selection->GetSelectedObject(Idx);
			if (a == this->GetOwner()) {
				bFound = true;
				break;
			}
		}
		if (bFound && Selection->Num() == 1) {
			OnSelectionChange.Broadcast(false);
		}
	}
#endif
}
void UVrmEditorEventComponent::OnSelectionObjectFunc(UObject *obj) {
#if WITH_EDITOR
	if (GEditor == nullptr) return;
	bool bFound = false;
	USelection* Selection = Cast<USelection>(obj);
	if (Selection == GEditor->GetSelectedComponents() || Selection == GEditor->GetSelectedActors()) {

		for (int32 Idx = 0; Idx < Selection->Num(); Idx++)
		{
			if (Selection->GetSelectedObject(Idx) == this->GetOwner()) {
				bFound = true;
				break;
			}
		}
	}
	if (bFound) {
		OnSelectionObject.Broadcast(false);
	}
#endif
}

void UVrmEditorEventComponent::SetSelectCheck(bool bCheckOn) {
#if WITH_EDITOR
	USelection::SelectionChangedEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionChangeFunc);
	USelection::SelectObjectEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionObjectFunc);
	//SelectObjectEvent
	if (bCheckOn) {
		//GEditor->OnEndCameraMovement.Assign(OnCameraMove)
		//if (GEditor->GetActiveViewport()) {
		//	FEditorViewportClient* ViewportClient = StaticCast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());
		//	if (ViewportClient) {
		//		ViewportClient->SetActorLock(this->GetOwner());
		//	}
		//}
		//GCurrentLevelEditingViewportClient->SetActorLock(this->GetOwner());
		//GEditor->OnEndCameraMovement().AddUObject(this, &UVrmCameraCheckComponent::OnCameraTransformChanged);
		//GEditor->OnBeginCameraMovement().AddUObject(this, &UVrmCameraCheckComponent::OnCameraTransformChanged);
		//handle = FEditorDelegates::OnEditorCameraMoved.AddUObject(this, &UVrmCameraCheckComponent::OnCameraTransformChanged);
	} else {
		//if (handle.IsValid()) {
			//FEditorDelegates::OnEditorCameraMoved.Remove(handle);
	//	}
		//GEditor->OnEndCameraMovement().Remove
	}
	//FOnEndTransformCamera& () { return OnEndCameraTransformEvent; }
#endif
}

void UVrmEditorEventComponent::OnGlobalTimeChangeFunc() {
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(4,26,0)
#elif UE_VERSION_OLDER_THAN(5,5,0)

	int32  t = ULevelSequenceEditorBlueprintLibrary::GetCurrentTime();
	OnGlobalTimeChange.Broadcast((float)t);
#else
	//int32  t = ULevelSequenceEditorBlueprintLibrary::GetCurrentTime();
	auto mm = ULevelSequenceEditorBlueprintLibrary::GetGlobalPosition();
	int32 t = mm.Timecode.Frames;
	OnGlobalTimeChange.Broadcast((float)t);
#endif
#endif
}


void UVrmEditorEventComponent::SetGlobalTimeCheck(bool bCheckOn) {
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
	{
		if (GEditor == nullptr) return;
		auto *LevelSeq = ULevelSequenceEditorBlueprintLibrary::GetCurrentLevelSequence();
		if (LevelSeq == nullptr) return;

		IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(LevelSeq, false);
		if (AssetEditor == nullptr) return;

		ILevelSequenceEditorToolkit* LevelSequenceEditor = static_cast<ILevelSequenceEditorToolkit*>(AssetEditor);

		if (LevelSequenceEditor == nullptr){
			return;
		}
		TWeakPtr<ISequencer> seq = LevelSequenceEditor->GetSequencer();



		if (seq.IsValid()) {
			seq.Pin()->OnGlobalTimeChanged().AddUObject(this, &UVrmEditorEventComponent::OnGlobalTimeChangeFunc);
		}
	}

	/*
	ALevelSequenceActor* LevelSequenceActor = nullptr;
	for (ALevelSequenceActor* ExistingLevelSequenceActor : TActorRange<ALevelSequenceActor>(MeshActor->GetWorld()))
	{
		LevelSequenceActor = ExistingLevelSequenceActor;
		break;
	}
	*/
	//auto *seq = ULevelSequenceEditorBlueprintLibrary::GetCurrentLevelSequence();
	//if (seq = nullptr) {
		//USelection::SelectionChangedEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionChangeFunc);
		//USelection::SelectObjectEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionObjectFunc);
	//}
	//seq->OnGlobalTimeChanged().AddRaw(this, &UVrmEditorEventComponent::OnGlobalTimeChangeFunc);
	//USelection::SelectionChangedEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionChangeFunc);
	//USelection::SelectObjectEvent.AddUObject(this, &UVrmEditorEventComponent::OnSelectionObjectFunc);
#endif
#endif
}



void UVrmEditorEventComponent::OnPIEEventFunc(EVRM4U_PIEEvent e) {
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
	OnPIEEvent.Broadcast(e);
#endif
#endif
}

void UVrmEditorEventComponent::OnBeginPIE(const bool bIsSimulating) {
	OnPIEEventFunc(EVRM4U_PIEEvent::BeginPIE);
}
void UVrmEditorEventComponent::OnEndPIE(const bool bIsSimulating) {
	OnPIEEventFunc(EVRM4U_PIEEvent::EndPIE);
}


void UVrmEditorEventComponent::SetPIEEventCheck(bool bCheckOn) {
#if WITH_EDITOR
#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
	{
		FEditorDelegates::BeginPIE.AddUObject(this, &UVrmEditorEventComponent::OnBeginPIE);
		FEditorDelegates::EndPIE.AddUObject(this, &UVrmEditorEventComponent::OnEndPIE);

	}
#endif
#endif
}

