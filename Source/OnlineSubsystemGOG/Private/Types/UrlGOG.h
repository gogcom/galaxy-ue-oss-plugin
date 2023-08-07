#pragma once

#include "Types/UniqueNetIdGOG.h"

#include "Engine/EngineBaseTypes.h"

class FUrlGOG : public FURL
{
public:

	explicit FUrlGOG(TCHAR const* InRemoteAddress)
		: FURL{nullptr, InRemoteAddress, ETravelType::TRAVEL_Absolute}
	{
		FString InRemoteAddressStr(InRemoteAddress);

		const FString LobbyConnectCmd = TEXT("+connect_lobby");
		int32 ConnectIdx = InRemoteAddressStr.Find(LobbyConnectCmd, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		if (ConnectIdx != INDEX_NONE)
		{
			const TCHAR* Str = InRemoteAddress + ConnectIdx + LobbyConnectCmd.Len();
			Host = FParse::Token(Str, 0);

			UE_LOG_ONLINE(Display, TEXT("lobbyID=%s"), *Host);
		}
	}

	explicit FUrlGOG(const FString& InRemoteAddress)
		: FURL{nullptr, *InRemoteAddress, ETravelType::TRAVEL_Absolute}
	{
	}

	explicit FUrlGOG(const FUniqueNetIdGOG& InRemoteID)
		: FUrlGOG{InRemoteID.ToString().Append(TEXT(".galaxy"))}
	{
	}

	bool operator==(const FURL& InOther) const
	{
		return FURL::operator==(InOther);
	}

	bool operator!=(const FURL& InOther) const
	{
		return !(*this == InOther);
	}

};
