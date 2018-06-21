#include "OnlineSessionSettingsConverter.h"

#include "SessionSettingsConverter.h"
#include "OnlineSessionSettingConverter.h"

#include "OnlineSubsystem.h"

#include <bitset>
#include <utility>

namespace OnlineSessionSettingsConverter
{

	namespace
	{

		using bit_flag_type = unsigned long;

		enum SessionSettingsBits : bit_flag_type
		{
			B_SHOULDADVERTISE = 0,
			B_ALLOWJOININPROGRESS,
			B_ISLANMATCH,
			B_ISDEDICATED,
			B_USESSTATS,
			B_ALLOWINVITES,
			B_USESPRESENCE,
			B_ALLOWJOINVIAPRESENCE,
			B_ALLOWJOINVIAPRESENCEFRIENDSONLY,
			B_ANTICHEATPROTECTED,

			TOTAL_COUNT
		};

		using SessionFlags = std::bitset<TOTAL_COUNT>;

		FLobbyDataEntry SerializeToLobbyDataEntry(SessionFlags InSessionFlags)
		{
			return FLobbyDataEntry::FromInt(InSessionFlags.to_ulong());
		}

		SessionFlags DeserializeFromLobbyDataEntry(const FLobbyDataEntry& InString)
		{
			return SessionFlags{static_cast<bit_flag_type>(FCString::Strtoui64(*InString, nullptr, 10))};
		}

	}

	FLobbyData ToLobbyData(const FOnlineSessionSettings& InSessionSettings)
	{
		FLobbyData convertedSettings;

		{
			auto sessionFlags = SessionFlags{};

			sessionFlags[B_SHOULDADVERTISE] = InSessionSettings.bShouldAdvertise;
			sessionFlags[B_ALLOWJOININPROGRESS] = InSessionSettings.bAllowJoinInProgress;
			sessionFlags[B_ISLANMATCH] = InSessionSettings.bIsLANMatch;
			sessionFlags[B_ISDEDICATED] = InSessionSettings.bIsDedicated;
			sessionFlags[B_USESSTATS] = InSessionSettings.bUsesStats;
			sessionFlags[B_ALLOWINVITES] = InSessionSettings.bAllowInvites;
			sessionFlags[B_USESPRESENCE] = InSessionSettings.bUsesPresence;
			sessionFlags[B_ALLOWJOINVIAPRESENCE] = InSessionSettings.bAllowJoinViaPresence;
			sessionFlags[B_ALLOWJOINVIAPRESENCEFRIENDSONLY] = InSessionSettings.bAllowJoinViaPresenceFriendsOnly;
			sessionFlags[B_ANTICHEATPROTECTED] = InSessionSettings.bAntiCheatProtected;

			convertedSettings.Emplace(lobby_data::SESSION_FLAGS, SerializeToLobbyDataEntry(sessionFlags));
		}

		convertedSettings.Emplace(lobby_data::BUILD_ID, FLobbyDataEntry::FromInt(InSessionSettings.BuildUniqueId));

		convertedSettings.Append(SessionSettingsConverter::ToLobbyData(InSessionSettings.Settings));

		return convertedSettings;
	}

	FOnlineSessionSettings FromLobbyData(const FLobbyData& InLobbyData)
	{
		FOnlineSessionSettings convertedSettings;

		for (const auto& lobbyDataRow : InLobbyData)
		{
			if (lobbyDataRow.Key == lobby_data::SESSION_FLAGS)
			{
				auto sessionFlags = DeserializeFromLobbyDataEntry(lobbyDataRow.Value);

				convertedSettings.bShouldAdvertise = sessionFlags[B_SHOULDADVERTISE];
				convertedSettings.bAllowJoinInProgress = sessionFlags[B_ALLOWJOININPROGRESS];
				convertedSettings.bIsLANMatch = sessionFlags[B_ISLANMATCH];
				convertedSettings.bIsDedicated = sessionFlags[B_ISDEDICATED];
				convertedSettings.bUsesStats = sessionFlags[B_USESSTATS];
				convertedSettings.bAllowInvites = sessionFlags[B_ALLOWINVITES];
				convertedSettings.bUsesPresence = sessionFlags[B_USESPRESENCE];
				convertedSettings.bAllowJoinViaPresence = sessionFlags[B_ALLOWJOINVIAPRESENCE];
				convertedSettings.bAllowJoinViaPresenceFriendsOnly = sessionFlags[B_ALLOWJOINVIAPRESENCEFRIENDSONLY];
				convertedSettings.bAntiCheatProtected = sessionFlags[B_ANTICHEATPROTECTED];

				continue;
			}

			if (lobbyDataRow.Key == lobby_data::BUILD_ID)
			{
				convertedSettings.BuildUniqueId = FCString::Atoi(*lobbyDataRow.Value);
				continue;
			}

			convertedSettings.Settings.Append(SessionSettingsConverter::FromLobbyDataRow(lobbyDataRow));
		}

		return std::move(convertedSettings);
	}

}
