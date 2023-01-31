#include "InternetAddrGOG.h"

FInternetAddrGOG::FInternetAddrGOG(FUniqueNetIdGOG InUniqueNetID)
	: uniqueNetID{MoveTemp(InUniqueNetID)}
{}

#if ENGINE_MINOR_VERSION >= 21 || ENGINE_MAJOR_VERSION > 4
TArray<uint8> FInternetAddrGOG::GetRawIp() const
{
	return {uniqueNetID.GetBytes(), uniqueNetID.GetSize()};
}

void FInternetAddrGOG::SetRawIp(const TArray<uint8>& InRawAddr)
{
	uniqueNetID = {InRawAddr.GetData(), InRawAddr.Num()};
}

TSharedRef<FInternetAddr> FInternetAddrGOG::Clone() const
{
	return MakeShared<FInternetAddrGOG>(uniqueNetID);
}

void FInternetAddrGOG::SetLoopbackAddress()
{
	checkf(false, TEXT("Not supported"));
}

bool FInternetAddrGOG::operator==(const FInternetAddr& InOther) const
{
	return GetRawIp() == InOther.GetRawIp();
}

#if ENGINE_MINOR_VERSION >= 22 || ENGINE_MAJOR_VERSION > 4
uint32 FInternetAddrGOG::GetTypeHash() const
#else
uint32 FInternetAddrGOG::GetTypeHash()
#endif
{
        return ::GetTypeHash(uniqueNetID);
}

#else
bool FInternetAddrGOG::operator==(const FInternetAddr& InOther) const
{
	return ToString(true) == InOther.ToString(true);
}
#endif

#if ENGINE_MINOR_VERSION >= 23 || ENGINE_MAJOR_VERSION > 4
FName FInternetAddrGOG::GetProtocolType() const
{
	return TEXT_GOG;
}
#endif
void FInternetAddrGOG::SetIp(uint32 /*InAddr*/)
{
	checkf(false, TEXT("Not supported"));
}

void FInternetAddrGOG::SetIp(const TCHAR* /*InAddr*/, bool& /*InIsValid*/)
{
	checkf(false, TEXT("Not supported"));
}

void FInternetAddrGOG::GetIp(uint32& /*OutAddr*/) const
{
	checkf(false, TEXT("Not supported"));
}

void FInternetAddrGOG::SetPort(int32 /*InPort*/)
{
	checkf(false, TEXT("Not supported"));
}

void FInternetAddrGOG::GetPort(int32& /*OutPort*/) const
{
	checkf(false, TEXT("Not supported"));
}

int32 FInternetAddrGOG::GetPort() const
{
	checkf(false, TEXT("Not supported"));
	return 0;
}

void FInternetAddrGOG::SetAnyAddress()
{
	checkf(false, TEXT("Not supported"));
}

void FInternetAddrGOG::SetBroadcastAddress()
{
	checkf(false, TEXT("Not supported"));
}

FString FInternetAddrGOG::ToString(bool /*InAppendPort*/) const
{
	return uniqueNetID.ToString();
}

bool FInternetAddrGOG::IsValid() const
{
	return uniqueNetID.IsValid();
}

