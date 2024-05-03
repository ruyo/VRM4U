// Fill out your copyright notice in the Description page of Project Settings.


#include "VRM4U_VMCSubsystem.h"

#include "UObject/StrongObjectPtr.h"
#include "OSCManager.h"
#include "OSCServer.h"

struct FCaptureData {
	TMap<FString, FTransform> BoneInfo;
	TMap<FString, float> BlendShapeInfo;
};

void UVRM4U_VMCSubsystem::OSCReceivedMessageEvent(const FOSCMessage& Message, const FString& IPAddress, uint16 Port) {

	UVRM4U_VMCSubsystem* subsystem = GEngine->GetEngineSubsystem<UVRM4U_VMCSubsystem>();
	if (subsystem == nullptr) return;

	auto data = subsystem->FindVMCData(IPAddress, (int)Port);
	if (data == nullptr) return;

	FOSCAddress a = UOSCManager::GetOSCMessageAddress(Message);
	FString addressPath = UOSCManager::GetOSCAddressFullPath(a);

	TArray<FString> str;
	UOSCManager::GetAllStrings(Message, str);

	TArray<float> curve;
	UOSCManager::GetAllFloats(Message, curve);
	FTransform t;
	float f = 0;
	if (curve.Num() == 1) {
		f = curve[0];
	}
	if (curve.Num() >= 7) {
		t.SetLocation(FVector(-curve[0], curve[2], curve[1]) * 100.f);
		t.SetRotation(FQuat(-curve[3], curve[5], curve[4], curve[6]));
	}
	if (curve.Num() >= 10) {
		t.SetScale3D(FVector(curve[7], curve[9], curve[8]));
	}

	if (addressPath == TEXT("/VMC/Ext/Bone/Pos")) {
		data->BoneData.FindOrAdd(str[0]) = t;
	}
	if (addressPath == TEXT("/VMC/Ext/Blend/Val")) {
		data->CurveData.FindOrAdd(str[0]) = f;
	}


	// 
	//GetAllFloats
}

FVMCData* UVRM4U_VMCSubsystem::FindVMCData(FString serverName, int port) {
	for (auto& s : ServerDataList) {
		if (s.ServerAddress == serverName) {
			return &s;
		}
	}
	return nullptr;
}

TStrongObjectPtr<UOSCServer> UVRM4U_VMCSubsystem::FindOrAddServer(const FString ServerAddress, int port) {
	if (port == 0) {
		return nullptr;
	}
	int ServerListNo = -1;
	{
		FVMCData d;
		d.ServerAddress = ServerAddress;
		d.Port = port;

		ServerListNo = ServerDataList.Find(d);
	}
	if (ServerListNo < 0) {
		ServerListNo = ServerDataList.AddDefaulted();
		ServerDataList[ServerListNo].ServerAddress = ServerAddress;
		ServerDataList[ServerListNo].Port = port;
		OSCServerList.AddDefaulted();
	}

	return OSCServerList[ServerListNo];
}

void UVRM4U_VMCSubsystem::DestroyVMCServer(const FString ServerAddress, int port) {

	auto OSCServer = FindOrAddServer(ServerAddress, port);
	if (OSCServer.Get()) {
		OSCServer->Stop();
	}
	OSCServer.Reset();
}


bool UVRM4U_VMCSubsystem::CreateVMCServer(const FString ServerAddress, int port) {

	auto OSCServer = FindOrAddServer(ServerAddress, port);
	if (OSCServer)
	{
		if (OSCServer->IsActive()) {
			return true;
		}
		OSCServer->Stop();
	}

	//const UVPUtilitiesEditorSettings* Settings = GetDefault<UVPUtilitiesEditorSettings>();
	//const FString& ServerAddress = "";// Settings->OSCServerAddress;
	uint16 ServerPort = port;// Settings->OSCServerPort;

	if (OSCServer)
	{
		OSCServer->SetAddress(ServerAddress, ServerPort);
		OSCServer->Listen();
	}
	else
	{
#if	UE_VERSION_OLDER_THAN(4,25,0)
#else
		OSCServer.Reset(UOSCManager::CreateOSCServer(ServerAddress, ServerPort, false, true, FString(), this));

#if WITH_EDITOR
		// Allow it to tick in editor, so that messages are parsed.
		// Only doing it upon creation so that the user can make it non-tickable if desired (and manage that thereafter).
		if (OSCServer)
		{
			OSCServer->SetTickInEditor(true);
		}
#endif // WITH_EDITOR

#endif
	}

	//void OSCReceivedMessageEvent(const FOSCMessage & Message, const FString & IPAddress, uint16 Port);
#if	UE_VERSION_OLDER_THAN(4,25,0)
#else
	//OSCServer->OnOscMessageReceivedNative.AddStatic(this, &UVRM4U_VMCSubsystem::OSCReceivedMessageEvent);
	OSCServer->OnOscMessageReceivedNative.RemoveAll(nullptr);
	OSCServer->OnOscMessageReceivedNative.AddStatic(&UVRM4U_VMCSubsystem::OSCReceivedMessageEvent);
#endif
	return true;
}

