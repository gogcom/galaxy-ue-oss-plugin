#pragma once

#include "Types/IListenerGOG.h"
#include "Types/UniqueNetIdGOG.h"

class FLobbyStartListener
	: public IListenerGOG
	, public galaxy::api::GlobalLobbyDataListener
{
public:

	FLobbyStartListener(const galaxy::api::GalaxyID& InLobbyID, const FName& InSessionName);

private:

	void OnLobbyDataUpdated(const galaxy::api::GalaxyID& InLobbyID, const galaxy::api::GalaxyID& InMemberID) override;

	bool MarkSessionStarted(bool IsJoinable) const;

	void TriggerOnStartSessionCompleteDelegates(bool InResult);

	const galaxy::api::GalaxyID lobbyID;
	const FName sessionName;
};
