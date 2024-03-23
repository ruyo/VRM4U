// VRM4U Copyright (c) 2021-2024 Haruyoshi Yamamoto. This software is released under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Misc/CoreDelegates.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Common/UdpSocketReceiver.h"
#include "Sockets.h"
#include "Common/UdpSocketBuilder.h"
#include "Misc/EngineVersionComparison.h"
#include "Tickable.h"


#if	UE_VERSION_OLDER_THAN(4,26,0)
#else
#include "Containers/RingBuffer.h"
#endif

#include "VrmMocopiReceiver.generated.h"

/**
 * 
 */

enum MocopiData{
	BoneNum = 27,
};


USTRUCT(BlueprintType)
struct FStructMocopiData{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	int FrameNo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	int Time = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "VRM4U")
	TArray<FTransform> MocopiTransformWorld;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "VRM4U")
	TArray<FTransform> MocopiTransformLocal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, EditFixedSize, Category = "VRM4U")
	TArray<FTransform> VrmTransformLocal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VRM4U")
	TMap<FString, FTransform> VrmTransformBoneList;

	FStructMocopiData() {
		MocopiTransformWorld.SetNum(MocopiData::BoneNum);
		MocopiTransformLocal.SetNum(MocopiData::BoneNum);
		VrmTransformLocal.SetNum(MocopiData::BoneNum);
	}
};

class VRM4UCAPTURE_API FMocopiReceiverProxy : public FTickableGameObject
{
	UVrmMocopiReceiver* Receiver;
	FSocket* Socket = nullptr;
	FUdpSocketReceiver* SocketReceiver = nullptr;

	FIPv4Address ReceiveIPAddress;
	int32 Port = 8888;

	TArray<uint8> RecvDataSet;

public:
	FMocopiReceiverProxy(UVrmMocopiReceiver *InReceiver);
	//FMocopiReceiverProxy(UVrmMocopiReceiver& InServer);
	virtual ~FMocopiReceiverProxy();

#if WITH_EDITOR
	virtual bool IsTickableInEditor() const override {
		return true;
	}
#endif // WITH_EDITOR

	void Listen();

	void Stop();

	bool SetAddress(const FString& ReceiveIPAddress, int32 Port);

	virtual void Tick(float InDeltaTime) override;
	virtual TStatId GetStatId() const override;

	void OnPacketReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InEndpoint);
};


UCLASS(BlueprintType)
class VRM4UCAPTURE_API UVrmMocopiReceiver : public UObject
{
	GENERATED_UCLASS_BODY()

	int BufferNum = 10;
	int currentFrameNo = 0;
	int currentTime = 0;

	mutable FCriticalSection BufferCS;

	TUniquePtr<FMocopiReceiverProxy> ReceiverProxy;

#if	UE_VERSION_OLDER_THAN(4,26,0)
	TArray<FStructMocopiData> MocopiReceiveBuffer;
#else
	TRingBuffer<FStructMocopiData> MocopiReceiveBuffer;
#endif

public:

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void Listen();

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void Stop();

	void OnPacketReceived(const FArrayReaderPtr& InData, const FIPv4Endpoint& InEndpoint);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	bool SetAddress(const FString& ReceiveIPAddress, int32 Port);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void SetBufferNum(int32 Num = 10);

	void OnPacketReceived(FStructMocopiData data);

	void PacketBroadcast();

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVrmMocopiReceiverDelegate, FStructMocopiData, data);

	UPROPERTY(BlueprintAssignable, Category = "VRM4U")
	FVrmMocopiReceiverDelegate OnReceived;

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void GetNextFrameData(FStructMocopiData &data, bool &bEnable, bool &bUpdate);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void GetLatestFrameData(FStructMocopiData &data, bool &bEnable, bool &bUpdate);

	UFUNCTION(BlueprintCallable, Category = "VRM4U")
	void GetCurrentTime(int& FrameNo, int& Time) {
		FrameNo = currentFrameNo;
		Time = currentTime;
	}

protected:
	virtual void BeginDestroy() override;


};
