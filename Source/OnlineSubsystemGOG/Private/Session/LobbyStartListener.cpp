#include "LobbyStartListener.h"

#include "OnlineSessionGOG.h"

#include "Online.h"

FLobbyStartListener::FLobbyStartListener(const galaxy::api::GalaxyID& InLobbyID, const FName& InSessionName)
	: lobbyID{InLobbyID}
	, sessionName{InSessionName}
{
	check(IsInGameThread());
}

void FLobbyStartListener::OnLobbyDataUpdated(const galaxy::api::GalaxyID& InLobbyID, const galaxy::api::GalaxyID& InMemberID)
{
	UE_LOG_ONLINE(Display, TEXT("OnLobbyDataUpdated: lobbyID=%llu"), InLobbyID.ToUint64());

	if (InLobbyID != lobbyID)
	{
		UE_LOG_ONLINE(Display, TEXT("Unknown lobby. Ignoring: updatedLobbyID=%llu"), InLobbyID.ToUint64());
		return;
	}

	if (InMemberID.IsValid())
	{
		UE_LOG_ONLINE(Display, TEXT("LobbyMemberData update. Ignoring"));
		return;
	}

	auto isLobbyJoinable = galaxy::api::Matchmaking()->IsLobbyJoinable(InLobbyID);
	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_ONLINE(Error, TEXT("Failed to check lobby joinability: %s, %s"), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));

	if (!isLobbyJoinable)
		UE_LOG_ONLINE(Error, TEXT("Failed to set Lobby as joinable"));

	TriggerOnStartSessionCompleteDelegates(isLobbyJoinable);
}

bool FLobbyStartListener::MarkSessionStarted(bool IsJoinable) const
{
	auto onlineSessionInterface = Online::GetSessionInterface();
	if (!onlineSessionInterface.IsValid())
	{
		return false;
	}

	auto storedSession = onlineSessionInterface->GetNamedSession(sessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to finalize session stating as OnlineSession interface is invalid: sessionName='%s'"), *sessionName.ToString());
		return false;
	}

	storedSession->SessionState = IsJoinable ? EOnlineSessionState::InProgress : EOnlineSessionState::Pending;
	return IsJoinable;
}

void FLobbyStartListener::TriggerOnStartSessionCompleteDelegates(bool InResult)
{
	auto onlineSessionInterface = StaticCastSharedPtr<FOnlineSessionGOG>(Online::GetSessionInterface());
	if (!onlineSessionInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to finalize session stating as OnlineSession interface is invalid: sessionName='%s'"), *sessionName.ToString());
		return;
	}

	onlineSessionInterface->TriggerOnStartSessionCompleteDelegates(sessionName, MarkSessionStarted(InResult));
	onlineSessionInterface->FreeListener(ListenerID);
}
