#pragma once

#include "Types/UniqueNetIdGOG.h"
#include "OnlineSessionSettings.h"

namespace OnlineSessionUtils
{

	bool Fill(const FUniqueNetIdGOG& InLobbyID, FOnlineSessionSettings& InOutOnlineSessionSettings);

	bool Fill(const FUniqueNetIdGOG& InLobbyID, FOnlineSession& InOutOnlineSession);

	bool Fill(const FUniqueNetIdGOG& InLobbyID, FOnlineSessionSearchResult& InOutOnlineSessionSearchResult);

}
