// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmNetworkBPFunctionLibrary.h"
#include "VRM4U.h"

#include "Engine/Engine.h"
#include "Logging/MessageLog.h"
#include "Misc/EngineVersionComparison.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <iphlpapi.h>
#include <winsock.h>

#include "Windows/HideWindowsPlatformTypes.h"
#endif

void UVrmNetworkBPFunctionLibrary::VRMGetIPAddressTable(TArray<FString> &AddressTable) {

	AddressTable.Empty();

#if PLATFORM_WINDOWS

	DWORD dwSize = 0;

	TArray<uint8> IPAddrTableBuffer;
	IPAddrTableBuffer.SetNum(sizeof(MIB_IPADDRTABLE));

	if (GetIpAddrTable(nullptr, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
		IPAddrTableBuffer.SetNum(dwSize);
	}

	auto dwRetVal = GetIpAddrTable(reinterpret_cast<MIB_IPADDRTABLE*>(IPAddrTableBuffer.GetData()), &dwSize, 0);
	if ( dwRetVal != NO_ERROR ) { 
		UE_LOG(LogVRM4U, Warning, TEXT("GetIpAddrTable call failed with %d\n"), dwRetVal);
	}

	{
		MIB_IPADDRTABLE* pIPAddrTable = reinterpret_cast<MIB_IPADDRTABLE*>(IPAddrTableBuffer.GetData());
		for (DWORD i = 0; i < pIPAddrTable->dwNumEntries; ++i) {
			AddressTable.Add(inet_ntoa(*(struct in_addr*)&pIPAddrTable->table[i].dwAddr));
		}
	}

#endif
}