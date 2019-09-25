#include "JoinLobbyListener.h"

#include "OnlineSessionGOG.h"
#include "LobbyData.h"

#include "Online.h"

FJoinLobbyListener::FJoinLobbyListener(
	class FOnlineSessionGOG& InSessionInterface,
	FName InSessionName,
	FOnlineSession InJoiningSession,
	TSharedRef<const FUniqueNetId> InOwnUserID
)
	: sessionInterface{InSessionInterface}
	, sessionName{MoveTemp(InSessionName)}
	, joiningSession{MoveTemp(InJoiningSession)}
	, ownUserID{MoveTemp(InOwnUserID)}
{
}

void FJoinLobbyListener::OnLobbyEntered(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyEnterResult InResult)
{
	UE_LOG_ONLINE_SESSION(Display, TEXT("OnLobbyEntered: lobbyID=%llu"), InLobbyID.ToUint64());

	if (InResult != galaxy::api::LOBBY_ENTER_RESULT_SUCCESS)
	{
		switch (InResult)
		{
			case galaxy::api::LOBBY_ENTER_RESULT_LOBBY_DOES_NOT_EXIST:
			{
				UE_LOG_ONLINE_SESSION(Error, TEXT("Lobby does not exists: lobbyID=%llu"), InLobbyID.ToUint64());

				TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::SessionDoesNotExist);
				return;
			}

			case galaxy::api::LOBBY_ENTER_RESULT_LOBBY_IS_FULL:
			{
				UE_LOG_ONLINE_SESSION(Display, TEXT("Lobby is full: lobbyID=%llu"), InLobbyID.ToUint64());

				TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::SessionIsFull);
				return;
			}


			case galaxy::api::LOBBY_ENTER_RESULT_ERROR:
			{
				UE_LOG_ONLINE_SESSION(Error, TEXT("Unknown error when entering lobby: lobbyID=%llu"), InLobbyID.ToUint64());

				TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::UnknownError);
				return;
			}
		}
	}

	// Save local copy of the session
	auto newSession = sessionInterface.AddNamedSession(sessionName, joiningSession);
	if (!newSession)
	{
		UE_LOG_ONLINE_SESSION(Error, TEXT("Failed to create new local session (NULL)"));
		return TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::UnknownError);
	}

	newSession->SessionState = EOnlineSessionState::Pending;
	newSession->HostingPlayerNum = LOCAL_USER_NUM;
	newSession->LocalOwnerId = MoveTemp(ownUserID);

	TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::Success);
}

void FJoinLobbyListener::TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::Type InResult)
{
	sessionInterface.TriggerOnJoinSessionCompleteDelegates(sessionName, InResult);
	sessionInterface.FreeListener(MoveTemp(ListenerID));
}
