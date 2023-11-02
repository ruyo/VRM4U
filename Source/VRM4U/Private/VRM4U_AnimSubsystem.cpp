// Fill out your copyright notice in the Description page of Project Settings.


#include "VRM4U_AnimSubsystem.h"


void UVRM4U_AnimSubsystem::Clear() {
	FScopeLock Lock(&cs);
	baseData.Empty();
}

void UVRM4U_AnimSubsystem::GetBoneByIndex(int index, TMap<FString, FTransform>& trans) {
	FScopeLock Lock(&cs);
	if (baseData.IsValidIndex(index)) {
		trans = baseData[index].BoneTransform;
	}
}

void UVRM4U_AnimSubsystem::GetBoneByPort(int port, TMap<FString, FTransform>& trans) {
	FScopeLock Lock(&cs);
	for (auto& a : baseData) {
		if (a.port == port) {
			trans = a.BoneTransform;
			return;
		}
	}
	FVrmTransformData a;
	a.port = port;
	baseData.Add(a);
	trans = baseData.Last(0).BoneTransform;
}

void UVRM4U_AnimSubsystem::GetRawdataByIndex(int index, TMap<FString, FTransform>& trans) {
	FScopeLock Lock(&cs);
	if (baseData.IsValidIndex(index)) {
		trans = baseData[index].RawData;
	}
}

void UVRM4U_AnimSubsystem::GetRawdataByPort(int port, TMap<FString, FTransform>& trans) {
	FScopeLock Lock(&cs);
	for (auto& a : baseData) {
		if (a.port == port) {
			trans = a.RawData;
			return;
		}
	}
	FVrmTransformData a;
	a.port = port;
	baseData.Add(a);
	trans = baseData.Last(0).RawData;
}

void UVRM4U_AnimSubsystem::SetBoneTransform(int port, const TMap<FString, FTransform>& trans) {
	FScopeLock Lock(&cs);
	for (auto& a : baseData) {
		if (a.port == port) {
			a.BoneTransform = trans;
			return;
		}
	}
	FVrmTransformData a;
	a.port = port;
	a.BoneTransform = trans;
	baseData.Add(a);
}

void UVRM4U_AnimSubsystem::SetRawData(int port, const TMap<FString, FTransform>& trans) {
	FScopeLock Lock(&cs);
	for (auto& a : baseData) {
		if (a.port == port) {
			a.RawData = trans;
			return;
		}
	}
	FVrmTransformData a;
	a.port = port;
	a.RawData = trans;
	baseData.Add(a);
}
