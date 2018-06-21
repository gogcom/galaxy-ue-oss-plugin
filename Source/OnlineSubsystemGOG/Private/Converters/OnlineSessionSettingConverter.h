#pragma once

#include "Session/LobbyData.h"

#include "OnlineSessionSettings.h"


namespace OnlineSessionSettingConverter
{

	FLobbyDataEntry ToLobbyDataEntry(const FOnlineSessionSetting& InSessionSetting);

	FOnlineSessionSetting FromLobbyDataEntry(const FLobbyDataEntry& InLobbyDataEntry);

}
