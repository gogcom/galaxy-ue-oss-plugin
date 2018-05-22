#pragma once

#include "OnlineKeyValuePair.h"

namespace lobby_data
{

	// See galaxy::api::Matchmaking::SetLobbyData()
	constexpr uint32_t MAX_KEY_LENGTH = 1023;

	// See galaxy::api::Matchmaking::SetLobbyData()
	constexpr uint32_t MAX_DATA_SIZE = 4095;

	const FName SESSION_OWNER_ID{TEXT("LD_S_OWNER_ID")};
	const FName SESSION_OWNER_NAME{TEXT("LD_S_OWNER_NAME")};
	const FName SESSION_FLAGS{TEXT("LD_S_FLAGS")};
	const FName BUILD_ID{TEXT("LD_BUILD_ID")};;

}

using FLobbyDataEntry = FString;
using FLobbyData = FOnlineKeyValuePairs<FName, FLobbyDataEntry>;
