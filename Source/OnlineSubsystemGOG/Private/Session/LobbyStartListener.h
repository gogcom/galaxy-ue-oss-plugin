#pragma once

#include "Types/IListenerGOG.h"
#include "Types/UniqueNetIdGOG.h"

class FLobbyStartListener
	: public IListenerGOG
	, public galaxy::api::ILobbyDataUpdateListener
{
public:

	FLobbyStartListener(galaxy::api::GalaxyID InLobbyID, FName InSessionName);

private:

	void OnLobbyDataUpdateSuccess(const galaxy::api::GalaxyID& InLobbyID) override;

	void OnLobbyDataUpdateFailure(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::ILobbyDataUpdateListener::FailureReason InFailureReason) override;

	bool MarkSessionStarted(bool IsJoinable) const;

	void TriggerOnStartSessionCompleteDelegates(bool InResult);

	const galaxy::api::GalaxyID lobbyID;
	const FName sessionName;
};
