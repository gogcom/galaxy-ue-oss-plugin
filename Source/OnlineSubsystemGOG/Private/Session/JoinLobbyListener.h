#pragma once

#include "Types/IListenerGOG.h"

#include "Types/UniqueNetIdGOG.h"

#include "OnlineSessionInterface.h"

#include "OnlineSessionSettings.h"

class FJoinLobbyListener
	: public IListenerGOG
	, public galaxy::api::GlobalLobbyEnteredListener
{
public:

	FJoinLobbyListener(const FUniqueNetIdGOG& InLobbyID, FName InSessionName, const FOnlineSession& InJoiningSession);

private:

	void OnLobbyEntered(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyEnterResult InResult) override;

	void TriggerOnJoinSessionCompleteDelegates(EOnJoinSessionCompleteResult::Type InResult);

	const FUniqueNetIdGOG lobbyID;
	const FName sessionName;
	const FOnlineSession joiningSession;
};
