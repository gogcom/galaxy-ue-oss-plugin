#include "JoinLobbyListener.h"

#include "OnlineSessionGOG.h"

#include "Online.h"

FJoinLobbyListener::FJoinLobbyListener(const FUniqueNetIdGOG& InLobbyID, FName InSessionName, const FOnlineSession& InJoiningSession)
	: lobbyID{InLobbyID}
	, sessionName{InSessionName}
	, joiningSession{InJoiningSession}
{
	check(IsInGameThread());
}

void FJoinLobbyListener::TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::Type InResult)
{
	auto onlineSessionInterface = StaticCastSharedPtr<FOnlineSessionGOG>(Online::GetSessionInterface());
	if (!onlineSessionInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to finalize session joining as OnlineSession interface is invalid: sessionName='%s'"), *sessionName.ToString());
		return;
	}

	// Save local copy of the session
	onlineSessionInterface->AddNamedSession(sessionName, joiningSession);

	onlineSessionInterface->TriggerOnJoinSessionCompleteDelegates(sessionName, InResult);

	onlineSessionInterface->FreeListener(ListenerID);
}

void FJoinLobbyListener::OnLobbyEntered(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyEnterResult InResult)
{
	UE_LOG_ONLINE(Display, TEXT("OnLobbyEntered: lobbyID=%llu"), InLobbyID.ToUint64());

	if (InResult != galaxy::api::LOBBY_ENTER_RESULT_SUCCESS)
	{
		switch (InResult)
		{
			case galaxy::api::LOBBY_ENTER_RESULT_LOBBY_DOES_NOT_EXIST:
			{
				UE_LOG_ONLINE(Error, TEXT("Lobby does not exists: lobbyID=%llu"), InLobbyID.ToUint64());

				TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::SessionDoesNotExist);
				return;
			}

			case galaxy::api::LOBBY_ENTER_RESULT_LOBBY_IS_FULL:
			{
				UE_LOG_ONLINE(Display, TEXT("Lobby is full: lobbyID=%llu"), InLobbyID.ToUint64());

				TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::SessionIsFull);
				return;
			}


			case galaxy::api::LOBBY_ENTER_RESULT_ERROR:
			{
				UE_LOG_ONLINE(Error, TEXT("Unknown error when entering lobby: lobbyID=%llu"), InLobbyID.ToUint64());

				TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::UnknownError);
				return;
			}
		}
	}

	TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::Success);
}