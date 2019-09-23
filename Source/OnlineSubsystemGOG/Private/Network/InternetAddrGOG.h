#pragma once

#include "Types/UniqueNetIdGOG.h"
#include "IPAddress.h"

class FInternetAddrGOG : public FInternetAddr
{
public:

	FInternetAddrGOG() = delete;

	explicit FInternetAddrGOG(const FInternetAddrGOG& InOther) = default;

	explicit FInternetAddrGOG(FUniqueNetIdGOG InUniqueNetID);

	void SetIp(uint32 InAddr) override;

	void SetIp(const TCHAR* InAddr, bool& OutIsValid) override;

	void GetIp(uint32& OutAddr) const override;

	void SetPort(int32 InPort) override;

	void GetPort(int32& OutPort) const override;

	int32 GetPort() const override;

	void SetAnyAddress() override;

	void SetBroadcastAddress() override;

	FString ToString(bool InAppendPort) const override;

	bool IsValid() const override;

	bool operator==(const FInternetAddr& InOther) const override;

#if ENGINE_MINOR_VERSION >= 21
	TArray<uint8> GetRawIp() const override;

	void SetRawIp(const TArray<uint8>& InRawAddr) override;

	TSharedRef<FInternetAddr> Clone() const override;

	void SetLoopbackAddress() override;
#if ENGINE_MINOR_VERSION >= 22

	uint32 GetTypeHash() const override;
#else

	uint32 GetTypeHash() override;
#endif
#endif

private:

	FUniqueNetIdGOG uniqueNetID;
};
