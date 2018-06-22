#pragma once

#include "Types/IListenerGOG.h"

#include "OnlineSubsystem.h"

#include "OnlineSessionSettings.h"

class FCreateLobbyListener
	: public IListenerGOG
	, public galaxy::api::GlobalLobbyCreatedListener
	, public galaxy::api::GlobalLobbyEnteredListener
	, public galaxy::api::GlobalLobbyDataListener
{
public:

	FCreateLobbyListener(FName InSessionName, FOnlineSessionSettings InSettings);

private:

	void OnLobbyCreated(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyCreateResult InResult) override;

	void OnLobbyEntered(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyEnterResult InResult)  override;

	void OnLobbyDataUpdated(const galaxy::api::GalaxyID& InLobbyID, const galaxy::api::GalaxyID& InMemberID) override;

	void TriggerOnCreateSessionCompleteDelegates(bool InIsSuccessful) const;

	const FName sessionName;
	FOnlineSessionSettings sessionSettings;

	galaxy::api::GalaxyID newLobbyID;
	galaxy::api::GalaxyID memberID;
};
