#include "GetSessionDetailsListener.h"

#include "OnlineSessionGOG.h"
#include "Converters/OnlineSessionSettingsConverter.h"
#include "LobbyData.h"
#include "VariantDataUtils.h"
#include "OnlineSessionUtils.h"

#include "Online.h"

FGetSessionDetailsListener::FGetSessionDetailsListener(
	FListenerManager& InListenerManager,
	FUniqueNetIdGOG InSessionID,
	FUniqueNetIdGOG InFriendID,
	FOnSingleSessionResultCompleteDelegate InCompletionDelegate)
	: listenerManager{InListenerManager}
	, sessionID{InSessionID}
	, friendID{InFriendID}
	, completionDelegate{InCompletionDelegate}
{
}

void FGetSessionDetailsListener::OnLobbyDataRetrieveSuccess(const galaxy::api::GalaxyID& InLobbyID)
{
	UE_LOG_ONLINE_SESSION(Display, TEXT("FGetSessionDetailsListener::OnLobbyDataRetrieveSuccess: lobbyID=%llu"), InLobbyID.ToUint64());

	FOnlineSessionSearchResult retrievedSession;
	if (!OnlineSessionUtils::Fill(InLobbyID, retrievedSession))
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Failed to get Session data: sessionID=%llu"), InLobbyID.ToUint64());

	TriggerOnSessionDetailsCompleteDelegate(true, retrievedSession);
}

void FGetSessionDetailsListener::OnLobbyDataRetrieveFailure(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::ILobbyDataRetrieveListener::FailureReason InFailureReason)
{
	UE_LOG_ONLINE_SESSION(Warning, TEXT("FGetSessionDetailsListener::OnLobbyDataRetrieveFailure: lobbyID=%llu"), InLobbyID.ToUint64());

	switch (InFailureReason)
	{
		case galaxy::api::ILobbyDataRetrieveListener::FAILURE_REASON_LOBBY_DOES_NOT_EXIST:
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("Session does not exist: sessionID=%llu"), InLobbyID.ToUint64());
			break;;
		}

		default:
			UE_LOG_ONLINE_SESSION(Warning, TEXT("Undefined error"));
			break;;
	}

	TriggerOnSessionDetailsCompleteDelegate(false);
}

void FGetSessionDetailsListener::TriggerOnSessionDetailsCompleteDelegate(bool InWasSuccessful, FOnlineSessionSearchResult InOnlineSessionSearchResult)
{
	completionDelegate.ExecuteIfBound(LOCAL_USER_NUM, InWasSuccessful, MoveTemp(InOnlineSessionSearchResult));
	listenerManager.FreeListener(MoveTemp(ListenerID));
}
