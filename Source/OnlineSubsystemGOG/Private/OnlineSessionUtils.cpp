#include "OnlineSessionUtils.h"

#include "CommonGOG.h"
#include "Converters/OnlineSessionSettingsConverter.h"
#include "Session/OnlineSessionInfoGOG.h"
#include "Session/LobbyData.h"

#include <array>

namespace
{

	bool GetSessionOwnerID(FOnlineSession& InOutOnlineSession)
	{
		auto ownerIdSetting = InOutOnlineSession.SessionSettings.Settings.Find(lobby_data::SESSION_OWNER_ID);
		if (!ownerIdSetting)
		{
			UE_LOG_ONLINE(Error, TEXT("Session owner ID not found"));
			return false;
		}

		FString sessionOwnerID;
		ownerIdSetting->Data.GetValue(sessionOwnerID);
		InOutOnlineSession.OwningUserId = MakeShared<FUniqueNetIdGOG>(sessionOwnerID);

		if (!InOutOnlineSession.OwningUserId->IsValid())
		{
			UE_LOG_ONLINE(Error, TEXT("Session owner ID is invalid"));
			return false;
		}

		return true;
	}

	bool GetSessionOwnerName(FOnlineSession& InOutOnlineSession)
	{
		auto ownerNameSetting = InOutOnlineSession.SessionSettings.Settings.Find(lobby_data::SESSION_OWNER_NAME);
		if (!ownerNameSetting)
		{
			UE_LOG_ONLINE(Error, TEXT("Session owner name not found"));
			return false;
		}

		ownerNameSetting->Data.GetValue(InOutOnlineSession.OwningUserName);
		if (InOutOnlineSession.OwningUserName.IsEmpty())
		{
			UE_LOG_ONLINE(Error, TEXT("Session owner name is empty"));
			return false;
		}

		return true;
	}

	bool GetSessionOpenConnections(const galaxy::api::GalaxyID& InLobbyID, FOnlineSession& InOutOnlineSession)
	{
		auto currentLobbySize = galaxy::api::Matchmaking()->GetNumLobbyMembers(InLobbyID);
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to get current lobby size: lobbyID=%llu; %s; %s"),
				InLobbyID.ToUint64(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return false;
		}

		InOutOnlineSession.NumOpenPublicConnections = std::max<int64>(InOutOnlineSession.SessionSettings.NumPublicConnections - currentLobbySize, 0u);
		InOutOnlineSession.NumOpenPrivateConnections = 0;
		return true;
	}

}

bool OnlineSessionUtils::Fill(const FUniqueNetIdGOG& InLobbyID, FOnlineSessionSettings& InOutOnlineSessionSettings)
{
	auto lobbyDataCount = galaxy::api::Matchmaking()->GetLobbyDataCount(InLobbyID);
	if (!lobbyDataCount)
	{
		UE_LOG_ONLINE(Error, TEXT("Lobby data not found: lobbyID='%s'"), *InLobbyID.ToString());
		return false;
	}

	FLobbyData lobbyData;

	std::array<char, lobby_data::MAX_KEY_LENGTH> lobbyDataKeyBuffer;
	std::array<char, lobby_data::MAX_DATA_SIZE> lobbyDataValueBuffer;

	for (decltype(lobbyDataCount) lobbyDataIdx = 0; lobbyDataIdx < lobbyDataCount; lobbyDataIdx++)
	{
		if (!galaxy::api::Matchmaking()->GetLobbyDataByIndex(
			InLobbyID,
			lobbyDataIdx,
			lobbyDataKeyBuffer.data(),
			lobbyDataKeyBuffer.size(),
			lobbyDataValueBuffer.data(),
			lobbyDataValueBuffer.size()))
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to fetch lobby data: lobbyID='%s'"), *InLobbyID.ToString());
			false;
		}

		lobbyData.Emplace(UTF8_TO_TCHAR(lobbyDataKeyBuffer.data()), UTF8_TO_TCHAR(lobbyDataValueBuffer.data()));
	}

	InOutOnlineSessionSettings = OnlineSessionSettingsConverter::FromLobbyData(lobbyData);

	auto maxLobbyMembers = galaxy::api::Matchmaking()->GetMaxNumLobbyMembers(InLobbyID);
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to get maximum lobby size: lobbyID='%s'; %s: %s"),
			*InLobbyID.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return false;
	}
	InOutOnlineSessionSettings.NumPublicConnections = maxLobbyMembers;
	check(InOutOnlineSessionSettings.NumPrivateConnections == 0);

	return true;
}

bool OnlineSessionUtils::Fill(const FUniqueNetIdGOG& InLobbyID, FOnlineSession& InOutOnlineSession)
{
	InOutOnlineSession.SessionInfo = MakeShared<FOnlineSessionInfoGOG>(InLobbyID);

	return Fill(InLobbyID, InOutOnlineSession.SessionSettings)
		&& GetSessionOwnerID(InOutOnlineSession)
		&& GetSessionOwnerName(InOutOnlineSession)
		&& GetSessionOpenConnections(InLobbyID, InOutOnlineSession);
}

bool OnlineSessionUtils::Fill(const FUniqueNetIdGOG& InLobbyID, FOnlineSessionSearchResult& InOutOnlineSessionSearchResult)
{
	InOutOnlineSessionSearchResult.PingInMs = 0;

	return Fill(InLobbyID, InOutOnlineSessionSearchResult.Session) && InOutOnlineSessionSearchResult.IsValid();
}
