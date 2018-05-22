#include "CreateLobbyListener.h"

#include "Online.h"
#include "Session/OnlineSessionGOG.h"

#include "OnlineSubsystemUtils.h"

FCreateLobbyListener::FCreateLobbyListener(
	const FName& InSessionName,
	const FOnlineSessionSettings& InSettings)
	: sessionName{InSessionName}
	, sessionSettings{InSettings}
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

	if (!SetLobbyData())
	{
		TriggerOnCreateSessionCompleteDelegates(false);
		return;
	}

	TriggerOnCreateSessionCompleteDelegates(true);
	// Wait until OnLobbyDataUpdated()
	// TODO: Implement OnLobbyDataUpdated() in "SDK - 2177: Employ session settings converters"
}

bool FCreateLobbyListener::SetLobbyData()
{
	// TODO: Implement in "SDK - 2177: Employ session settings converters"
	return true;
}

void FCreateLobbyListener::TriggerOnCreateSessionCompleteDelegates(bool InIsSuccessful) const
{
	auto onlineSessionInterface = StaticCastSharedPtr<FOnlineSessionGOG>(Online::GetSessionInterface());
	if (!onlineSessionInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to finalize session creation as OnlineSession interface is invalid: sessionName='%s'"), *sessionName.ToString());
		return;
	}

	if (InIsSuccessful)
	{
		auto newSession = dynamic_cast<FOnlineSessionGOG*>(onlineSessionInterface.Get())->AddNamedSession(sessionName, sessionSettings);

		newSession->SessionState = EOnlineSessionState::Pending;
		newSession->SessionInfo = MakeShared<FOnlineSessionInfoGOG>(newLobbyID);
	}

	onlineSessionInterface->TriggerOnCreateSessionCompleteDelegates(sessionName, InIsSuccessful);

	onlineSessionInterface->FreeListener(ListenerID);
}
