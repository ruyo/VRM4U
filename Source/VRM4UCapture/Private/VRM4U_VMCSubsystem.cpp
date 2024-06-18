// Fill out your copyright notice in the Description page of Project Settings.


#include "VRM4U_VMCSubsystem.h"
#include "VrmVMCObject.h"

#include "Engine/Engine.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/ScopeLock.h"
#include "OSCManager.h"
#include "OSCServer.h"

bool UVRM4U_VMCSubsystem::CopyVMCData(FVMCData &data, FString ServerAddress, int port) {
	for (int i = 0; i < VMCObjectList.Num(); ++i) {
		auto a = VMCObjectList[i].Get();
		if (a == nullptr) continue;

		if (a->ServerName == ServerAddress && a->port == port) {
			a->CopyVMCData(data);
			return true;
		}
	}
	return false;
}

bool UVRM4U_VMCSubsystem::GetVMCData(TMap<FString, FTransform>& BoneData, TMap<FString, float>& CurveData, FString ServerAddress, int port) {
	FVMCData d;
	if (CopyVMCData(d, ServerAddress, port) == false) {
		return false;
	}

	BoneData = d.BoneData;
	CurveData = d.CurveData;

	return true;
}


bool UVRM4U_VMCSubsystem::FindOrAddServer(const FString ServerAddress, int port) {

	bool bFound = false;
	for (int i = 0; i < VMCObjectList.Num(); ++i) {
		auto a = VMCObjectList[i].Get();
		if (a == nullptr) continue;

		if (a->ServerName == ServerAddress && a->port == port) {
			bFound = true;
			break;
		}
	}
	if (bFound == false) {
		int ind = VMCObjectList.AddDefaulted();
		VMCObjectList[ind].Reset(NewObject<UVrmVMCObject>());
		VMCObjectList[ind]->CreateServer(ServerAddress, port);
	}
	return true;
}

void UVRM4U_VMCSubsystem::DestroyVMCServer(const FString ServerAddress, int port) {
	for (int i = 0; i < VMCObjectList.Num(); ++i) {
		auto a = VMCObjectList[i].Get();
		if (a == nullptr) continue;

		if (a->ServerName == ServerAddress && a->port == port) {
			VMCObjectList[i].Reset(nullptr);
			VMCObjectList.RemoveAt(i);
			return;
		}
	}
}


bool UVRM4U_VMCSubsystem::CreateVMCServer(const FString ServerAddress, int port) {
	return FindOrAddServer(ServerAddress, port);
}

void UVRM4U_VMCSubsystem::ClearData(const FString ServerAddress, int port) {
	for (int i = 0; i < VMCObjectList.Num(); ++i) {
		auto a = VMCObjectList[i].Get();
		if (a == nullptr) continue;

		if (a->ServerName == ServerAddress && a->port == port) {
			VMCObjectList[i]->ClearVMCData();
			return;
		}
	}
}
