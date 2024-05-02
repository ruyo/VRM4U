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
	FOSCAddress a = UOSCManager::GetOSCMessageAddress(Message);
	FString s = UOSCManager::GetOSCAddressFullPath(a);

	// /VMC/Ext/Bone/Pos
	// /VMC/Ext/Blend/Val
	// 
	TArray<FString> str;

	UOSCManager::GetAllStrings(Message, str);

	TArray<float> curve;
	UOSCManager::GetAllFloats(Message, curve);


	// 
	//GetAllFloats
}

bool UVRM4U_VMCSubsystem::CreateVMCServer(const FString ServerAddress, int port) {
	int ServerListNo = -1;
	{
		FVMCServerData d;
		d.ServerAddress = ServerAddress;
		d.Port = port;

		ServerListNo = ServerDataList.Find(d);
	}
	if (ServerListNo < 0) {
		ServerListNo = ServerDataList.AddDefaulted();
	}

	if (ServerListNo < 0) {
		return false;
	}

	auto& OSCServer = OSCServerList[ServerListNo];

	if (OSCServer)
	{
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
		//OSCServer.Reset(UOSCManager::CreateOSCServer(ServerAddress, ServerPort, false, true, FString(), GetTransientPackage()));
		OSCServer.Reset(UOSCManager::CreateOSCServer(ServerAddress, ServerPort, false, true, FString()));

#if WITH_EDITOR
		// Allow it to tick in editor, so that messages are parsed.
		// Only doing it upon creation so that the user can make it non-tickable if desired (and manage that thereafter).
		if (OSCServer)
		{
			OSCServer->SetTickInEditor(true);
		}
#endif // WITH_EDITOR
	}

	//void OSCReceivedMessageEvent(const FOSCMessage & Message, const FString & IPAddress, uint16 Port);
#if	UE_VERSION_OLDER_THAN(4,25,0)
#else
	//OSCServer->OnOscMessageReceivedNative.AddStatic(this, &UVRM4U_VMCSubsystem::OSCReceivedMessageEvent);
	OSCServer->OnOscMessageReceivedNative.AddStatic(&UVRM4U_VMCSubsystem::OSCReceivedMessageEvent);
#endif
	return true;
}

