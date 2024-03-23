// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#include "VrmCaptureManager.h"
#include "VrmMocopiReceiver.h"
#include "VRM4UCaptureLog.h"

UVrmMocopiReceiver* UVrmCaptureManager::CreateVrmMocopiReceiver(FString InReceiveIPAddress, int32 InPort, bool bInMulticastLoopback, bool bInStartListening, FString ServerName, UObject* Outer)
{
	{
		FString InAddress = InReceiveIPAddress;
		if (!InAddress.IsEmpty() && InAddress != TEXT("0"))
		{
		}

		bool bCanBind = false;
		bool bAppendPort = false;
		if (ISocketSubsystem* SocketSys = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
		{
			const TSharedPtr<FInternetAddr> Addr = SocketSys->GetLocalHostAddr(*GLog, bCanBind);
			if (Addr.IsValid())
			{
				InAddress = Addr->ToString(bAppendPort);
			}
		}
		UE_LOG(LogVRM4UCapture, Display, TEXT("MocopiReceiver ReceiveAddress not specified. Using LocalHost IP: '%s'"), *InAddress);
	}

	if (ServerName.IsEmpty())
	{
		ServerName = FString::Printf(TEXT("MocopiReceiver_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	UVrmMocopiReceiver* NewMocopiServer = nullptr;
	if (Outer)
	{
		NewMocopiServer = NewObject<UVrmMocopiReceiver>(Outer, *ServerName, RF_StrongRefOnFrame);
	}
	else
	{
		UE_LOG(LogVRM4UCapture, Warning, TEXT("Outer object not set.  MocopiReceiver may be garbage collected if not referenced."));
		NewMocopiServer = NewObject<UVrmMocopiReceiver>(GetTransientPackage(), *ServerName);
	}

	if (NewMocopiServer)
	{
		//NewMocopiServer->SetMulticastLoopback(bInMulticastLoopback);
		if (NewMocopiServer->SetAddress(InReceiveIPAddress, InPort))
		{
			if (bInStartListening)
			{
				NewMocopiServer->Listen();
			}
		}
		else
		{
			UE_LOG(LogVRM4UCapture, Warning, TEXT("Failed to parse ReceiveAddress '%s' for MocopiReceiver."), *InReceiveIPAddress);
		}

		return NewMocopiServer;
	}

	return nullptr;
}

