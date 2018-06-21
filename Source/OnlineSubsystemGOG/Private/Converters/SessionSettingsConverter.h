#pragma once

#include "Session/LobbyData.h"

#include "OnlineSessionSettings.h"

namespace SessionSettingsConverter
{

	FLobbyData ToLobbyData(const FSessionSettings& InSessionSettings);

	FSessionSettings FromLobbyDataRow(const FLobbyData::ElementType& InLobbyData);
}
