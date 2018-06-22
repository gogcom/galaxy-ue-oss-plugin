#include "CreateLobbyListener.h"

#include "LobbyData.h"
#include "Session/OnlineSessionGOG.h"
#include "Converters/OnlineSessionSettingsConverter.h"

#include "Online.h"
#include "OnlineSubsystemUtils.h"

namespace
{

	bool AppendOwnerData(FOnlineSessionSettings& InOutSessionSettings)
	{
		// Store Session owner name and ID to avoid additional Galaxy async request

		auto onlineIdentityInterface = Online::GetIdentityInterface();
		if (!onlineIdentityInterface.IsValid())
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to get Session owner data. OnlineIdentity interface is NULL"));
			return false;
		}

		auto sessionOwnerName = onlineIdentityInterface->GetPlayerNickname(LOCAL_USER_NUM);
		if (sessionOwnerName.IsEmpty())
		{
			UE_LOG_ONLINE(Error, TEXT("Session owner name is empty"));
			return false;
		}

		auto sessionOwnerID = onlineIdentityInterface->GetUniquePlayerId(LOCAL_USER_NUM);
		if (!sessionOwnerID.IsValid() || !sessionOwnerID->IsValid())
			return false;

		InOutSessionSettings.Settings.Emplace(lobby_data::SESSION_OWNER_NAME, FOnlineSessionSetting{sessionOwnerName, EOnlineDataAdvertisementType::ViaOnlineService});
		InOutSessionSettings.Settings.Emplace(lobby_data::SESSION_OWNER_ID, FOnlineSessionSetting{sessionOwnerID->ToString(), EOnlineDataAdvertisementType::ViaOnlineService});

		return true;
	}

	bool SetLobbyData(const galaxy::api::GalaxyID& InLobbyID,  FOnlineSessionSettings& InOutSessionSettings)
	{
		if (!AppendOwnerData(InOutSessionSettings))
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to set Session owner data"));
			return false;
		}

		for (auto& lobbySetting : OnlineSessionSettingsConverter::ToLobbyData(InOutSessionSettings))
		{
			if (!galaxy::api::Matchmaking()->SetLobbyData(InLobbyID, TCHAR_TO_UTF8(*lobbySetting.Key.ToString()), TCHAR_TO_UTF8(*lobbySetting.Value)))
			{
				UE_LOG_ONLINE(Error, TEXT("Failed to set lobby setting: lobbyID=%llu, settingName='%s'"),
					InLobbyID.ToUint64(), *lobbySetting.Key.ToString());

				return false;
			}
		}

		return true;
	}

	bool AddLocalSession(const galaxy::api::GalaxyID& InLobbyID, const FName& InSessionName, const FOnlineSessionSettings& InSessionSettings)
	{
		auto onlineSessionInterface = StaticCastSharedPtr<FOnlineSessionGOG>(Online::GetSessionInterface());
		if (!onlineSessionInterface.IsValid())
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to add local session as OnlineSession interface is invalid"));
			return false;
		}

		auto newSession = dynamic_cast<FOnlineSessionGOG*>(onlineSessionInterface.Get())->AddNamedSession(InSessionName, InSessionSettings);
		if (!newSession)
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to create new local session (NULL)"));
			return false;
		}

		newSession->SessionState = EOnlineSessionState::Pending;
		newSession->SessionInfo = MakeShared<FOnlineSessionInfoGOG>(InLobbyID);

		return true;
	}

	bool MakeSessionJoinable(const galaxy::api::GalaxyID& InLobbyID)
	{
		if (!galaxy::api::Matchmaking()->SetLobbyJoinable(InLobbyID, true))
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to make session joinable: lobbyID=%llu"), InLobbyID.ToUint64());
			return false;
		}

		return true;
	}
}

FCreateLobbyListener::FCreateLobbyListener(FName InSessionName, FOnlineSessionSettings InSettings)
	: sessionName{std::move(InSessionName)}
	, sessionSettings{std::move(InSettings)}
{
	check(IsInGameThread());
}

void FCreateLobbyListener::OnLobbyCreated(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyCreateResult InResult)
{
	UE_LOG_ONLINE(Display, TEXT("FCreateLobbyListener::OnLobbyCreated: lobbyID=%llu, result=%d"), InLobbyID.ToUint64(), static_cast<int>(InResult));

	if (InResult != galaxy::api::LOBBY_CREATE_RESULT_SUCCESS)
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to create a lobby: lobbyID=%llu"), newLobbyID.ToUint64());

		TriggerOnCreateSessionCompleteDelegates(false);
		return;
	}

	check(InLobbyID.IsValid());
	newLobbyID = InLobbyID;

	// Wait till OnLobbyEntered
}

void FCreateLobbyListener::OnLobbyEntered(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyEnterResult InResult)
{
	UE_LOG_ONLINE(Display, TEXT("FCreateLobbyListener::OnLobbyEntered: lobbyID=%llu, result=%d"), InLobbyID.ToUint64(), static_cast<int>(InResult));

	if (newLobbyID != InLobbyID)
	{
		UE_LOG_ONLINE(Display, TEXT("Unknown lobby, skipping: inLobbyID=%llu, storedLobbyID=%llu"), InLobbyID.ToUint64(), newLobbyID.ToUint64());
		return;
	}

	if (InResult != galaxy::api::LOBBY_ENTER_RESULT_SUCCESS)
	{

		switch (InResult)
		{
			case galaxy::api::LOBBY_ENTER_RESULT_LOBBY_DOES_NOT_EXIST:
			{
				UE_LOG_ONLINE(Error, TEXT("Lobby does not exists: lobbyID=%llu"), InLobbyID.ToUint64());
				break;
			}

			case galaxy::api::LOBBY_ENTER_RESULT_LOBBY_IS_FULL:
			{
				UE_LOG_ONLINE(Display, TEXT("Lobby is full: lobbyID=%llu"), InLobbyID.ToUint64());
				break;
			}

			case galaxy::api::LOBBY_ENTER_RESULT_ERROR:
			{
				UE_LOG_ONLINE(Error, TEXT("Unknown error when entering lobby: lobbyID=%llu"), InLobbyID.ToUint64());
				break;
			}
		}

		TriggerOnCreateSessionCompleteDelegates(false);
		return;
	}

	if (!SetLobbyData(newLobbyID, sessionSettings))
	{
		TriggerOnCreateSessionCompleteDelegates(false);
		return;
	}

	// Wait until OnLobbyDataUpdated()
}

void FCreateLobbyListener::OnLobbyDataUpdated(const galaxy::api::GalaxyID& InLobbyID, const galaxy::api::GalaxyID& InMemberID)
{
	UE_LOG_ONLINE(Display, TEXT("FCreateLobbyListener::OnLobbyDataUpdated: lobbyID=%llu"), InLobbyID.ToUint64());

	if (InLobbyID != newLobbyID)
	{
		UE_LOG_ONLINE(Display, TEXT("Unknown lobby. Skipping: updatedLobbyID=%llu, lobbyID=%llu"), InLobbyID.ToUint64(), newLobbyID.ToUint64());
		return;
	}

	if (InMemberID.IsValid())
	{
		UE_LOG_ONLINE(Display, TEXT("Got LobbyMemberData while waiting for LobbyData update. Skipping: lobbyMemberID=%llu, lobbyID=%llu"), InMemberID.ToUint64(), newLobbyID.ToUint64());
		return;
	}

	auto sessionCreationResult = AddLocalSession(newLobbyID, sessionName, sessionSettings) && MakeSessionJoinable(newLobbyID);
	TriggerOnCreateSessionCompleteDelegates(sessionCreationResult);
}

void FCreateLobbyListener::TriggerOnCreateSessionCompleteDelegates(bool InIsSuccessful) const
{
	auto onlineSessionInterface = StaticCastSharedPtr<FOnlineSessionGOG>(Online::GetSessionInterface());
	if (!onlineSessionInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to finalize session creation as OnlineSession interface is invalid"));
		galaxy::api::Matchmaking()->LeaveLobby(newLobbyID);
		return;
	}

	if (!InIsSuccessful)
	{
		galaxy::api::Matchmaking()->LeaveLobby(newLobbyID);
		onlineSessionInterface->RemoveNamedSession(sessionName);
	}

	onlineSessionInterface->TriggerOnCreateSessionCompleteDelegates(sessionName, InIsSuccessful);

	onlineSessionInterface->FreeListener(ListenerID);
}
