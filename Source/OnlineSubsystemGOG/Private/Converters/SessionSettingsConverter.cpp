#include "SessionSettingsConverter.h"

#include "OnlineSessionSettingConverter.h"

namespace SessionSettingsConverter
{

	FLobbyData ToLobbyData(const FSessionSettings& InSessionSettings)
	{
		FLobbyData convertedSettings;

		for (const auto& setting : InSessionSettings)
		{
			if (setting.Value.AdvertisementType == EOnlineDataAdvertisementType::ViaOnlineService
				|| setting.Value.AdvertisementType == EOnlineDataAdvertisementType::ViaOnlineServiceAndPing)
			{
				convertedSettings.Emplace(setting.Key, OnlineSessionSettingConverter::ToLobbyDataEntry(setting.Value.Data));
			}
		}

		return convertedSettings;
	}

	FSessionSettings FromLobbyDataRow(const FLobbyData::ElementType& InLobbyData)
	{
		FSessionSettings ret;
		ret.Emplace(InLobbyData.Key, OnlineSessionSettingConverter::FromLobbyDataEntry(InLobbyData.Value));
		return ret;
	}

}
