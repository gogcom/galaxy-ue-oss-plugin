#pragma once

#include "Types/IListenerGOG.h"

#include "OnlineSubsystem.h"

#include "OnlineSessionSettings.h"

class FCreateLobbyListener
	: public IListenerGOG
	, public galaxy::api::ILobbyCreatedListener
	, public galaxy::api::ILobbyEnteredListener
	, public galaxy::api::ILobbyDataUpdateListener
{
public:

	FCreateLobbyListener(class FOnlineSessionGOG& InSessionInterface, FName InSessionName, FOnlineSessionSettings InSettings);

private:

	void OnLobbyCreated(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyCreateResult InResult) override;

	void OnLobbyEntered(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::LobbyEnterResult InResult)  override;

	void OnLobbyDataUpdateSuccess(const galaxy::api::GalaxyID& InLobbyID) override;

	void OnLobbyDataUpdateFailure(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::ILobbyDataUpdateListener::FailureReason InFailureReason) override;

	void TriggerOnCreateSessionCompleteDelegates(bool InIsSuccessful) const;

	class FOnlineSessionGOG& sessionInterface;
	const FName sessionName;
	FOnlineSessionSettings sessionSettings;

	galaxy::api::GalaxyID newLobbyID;
	galaxy::api::GalaxyID memberID;
};
