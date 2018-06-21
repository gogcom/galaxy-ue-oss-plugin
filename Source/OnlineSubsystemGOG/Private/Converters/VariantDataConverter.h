#pragma once

#include "Session/LobbyData.h"

namespace VariantDataConverter
{

	FLobbyDataEntry ToLobbyDataEntry(const FVariantData& InData);

	FVariantData FromLobbyDataEntry(const FLobbyDataEntry& InDataEntry);
}
