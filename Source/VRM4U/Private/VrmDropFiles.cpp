// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.
// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
// ApplicationLifecycleComponent.cpp: Component to handle receiving notifications from the OS about application state (activated, suspended, termination, etc)

#include "VrmDropFiles.h"
#include "Misc/CoreDelegates.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "commdlg.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

UVrmDropFilesComponent::FStaticOnDropFiles	UVrmDropFilesComponent::StaticOnDropFilesDelegate;
UVrmDropFilesComponent *UVrmDropFilesComponent::s_LatestActiveComponent = nullptr;


UVrmDropFilesComponent::UVrmDropFilesComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UVrmDropFilesComponent::OnRegister()
{
	Super::OnRegister();

	/*
	FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationWillDeactivateDelegate_Handler);
	FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationHasReactivatedDelegate_Handler);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationWillEnterBackgroundDelegate_Handler);
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationHasEnteredForegroundDelegate_Handler);
	FCoreDelegates::ApplicationWillTerminateDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationWillTerminateDelegate_Handler);
	FCoreDelegates::ApplicationShouldUnloadResourcesDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationShouldUnloadResourcesDelegate_Handler);
	FCoreDelegates::ApplicationReceivedStartupArgumentsDelegate.AddUObject(this, &UApplicationLifecycleComponent::ApplicationReceivedStartupArgumentsDelegate_Handler);

	FCoreDelegates::OnTemperatureChange.AddUObject(this, &UApplicationLifecycleComponent::OnTemperatureChangeDelegate_Handler);
	FCoreDelegates::OnLowPowerMode.AddUObject(this, &UApplicationLifecycleComponent::OnLowPowerModeDelegate_Handler);
	*/
	StaticOnDropFilesDelegate.AddUObject(this, &UVrmDropFilesComponent::OnDropFilesDelegate_Handler);

	s_LatestActiveComponent = this;
}

void UVrmDropFilesComponent::OnUnregister()
{
	Super::OnUnregister();

	/*
 	FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationHasReactivatedDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationWillTerminateDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationShouldUnloadResourcesDelegate.RemoveAll(this);
 	FCoreDelegates::ApplicationReceivedStartupArgumentsDelegate.RemoveAll(this);
	FCoreDelegates::OnTemperatureChange.RemoveAll(this);
	FCoreDelegates::OnLowPowerMode.RemoveAll(this);
	*/

	StaticOnDropFilesDelegate.RemoveAll(this);

	if (s_LatestActiveComponent == this) {
		s_LatestActiveComponent = nullptr;
	}
}

bool UVrmDropFilesComponent::VRMGetOpenFileName(FString &FileName) {
	FileName = "";

#if PLATFORM_WINDOWS

	OPENFILENAME    ofn = {};
	TCHAR filename[MAX_PATH] = {};

	filename[0] = '\0';
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	//ofn.hwndOwner = hWnd;
	ofn.lpstrFilter =
		TEXT("VRM file(*.vrm)\0*.vrm\0")
		TEXT("all file(*.*)\0*.*\0\0");
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	//ofn.lpstrDefExt = TEXT("*");
	ofn.lpstrTitle = TEXT("Model");

	if (GetOpenFileName(&ofn)) {
		FileName = filename;
		return true;
	}

#endif
	return false;

}
