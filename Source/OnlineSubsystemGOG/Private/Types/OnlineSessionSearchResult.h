#pragma once

#include "UniqueNetIdGOG.h"
#include "OnlineSessionSettings.h"

namespace OnlineSessionSearchResult
{

	bool Fill(const FUniqueNetIdGOG& InLobbyID, FOnlineSessionSearchResult& InOutOnlineSessionSearchResult);

}
