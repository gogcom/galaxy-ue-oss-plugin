#include "OnlineSessionSettingConverter.h"

#include "VariantDataConverter.h"

namespace OnlineSessionSettingConverter
{

	FLobbyDataEntry ToLobbyDataEntry(const FOnlineSessionSetting& InSessionSetting)
	{
		return {VariantDataConverter::ToLobbyDataEntry(InSessionSetting.Data)};
	}

	FOnlineSessionSetting FromLobbyDataEntry(const FLobbyDataEntry& InLobbyDataEntry)
	{
		return {VariantDataConverter::FromLobbyDataEntry(InLobbyDataEntry), EOnlineDataAdvertisementType::ViaOnlineService};
	}

}
