#pragma once

#include "CommonGOG.h"
#include "Types/UniqueNetIdGOG.h"

#include "IPAddress.h"
#include "Engine/EngineBaseTypes.h"

class FInternetAddrGOG : public FInternetAddr
{
public:

	explicit FInternetAddrGOG(FUniqueNetIdGOG InUniqueNetID)
		: netIdGOG{std::move(InUniqueNetID)}
	{
	}

	explicit FInternetAddrGOG(const FURL& InConnectURL)
		: netIdGOG{InConnectURL.Host}
	{
	}

	void SetIp(uint32 InAddr) override
	{
		// Not used
	}

	void SetIp(const TCHAR* InAddr, bool& bIsValid) override
	{
		// Not used
	}

	void GetIp(uint32& OutAddr) const override
	{
		// Not used
	}

	void SetPort(int32 InPort) override
	{
		// Not used
	}

	void GetPort(int32& OutPort) const override
	{
		// Not used
	}

	int32 GetPort() const override
	{
		// Not used
		return 0;
	}

	void SetAnyAddress() override
	{
		// Not used
	}

	void SetBroadcastAddress() override
	{
		// Not used
	}

	FString ToString(bool bAppendPort) const override
	{
		return netIdGOG.ToString().Append(TEXT(".galaxy"));
	}

	bool operator==(const FInternetAddr& InOther) const override
	{
		return dynamic_cast<const FInternetAddrGOG&>(InOther).netIdGOG == netIdGOG;
	}

	bool operator!=(const FInternetAddr& InOther) const
	{
		return !(dynamic_cast<const FInternetAddrGOG&>(InOther).netIdGOG == netIdGOG);
	}

	bool IsValid() const override
	{
		return netIdGOG.IsValid();
	}

	FUniqueNetIdGOG GetUniqueNetIdGOG() const
	{
		return netIdGOG;
	}

private:

	FUniqueNetIdGOG netIdGOG;
};
