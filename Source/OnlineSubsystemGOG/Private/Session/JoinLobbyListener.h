#pragma once

#include "Types/IListenerGOG.h"

#include "Types/UniqueNetIdGOG.h"

#include "Interfaces/OnlineSessionInterface.h"

#include "OnlineSessionSettings.h"

class FJoinLobbyListener
	: public IListenerGOG
	, public SelfDeregisteringListenerGOG<galaxy::api::ILobbyEnteredListener>
{
public:

	FJoinLobbyListener(
		class FOnlineSessionGOG& InSessionInterface,
		FName InSessionName,
		FOnlineSession InJoiningSession,
		TSharedRef<const FUniqueNetId> InOwnUserID);

private:

	void OnLobbyEntered(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyEnterResult InResult) override;

	void TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::Type InResult);

	class FOnlineSessionGOG& sessionInterface;
	const FName sessionName;
	const FOnlineSession joiningSession;
	TSharedRef<const FUniqueNetId> ownUserID;
};
