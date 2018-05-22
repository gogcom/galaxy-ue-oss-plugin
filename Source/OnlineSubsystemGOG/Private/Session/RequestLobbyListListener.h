#pragma once

#include "Types/IListenerGOG.h"

#include "OnlineSessionSettings.h"

class FRequestLobbyListListener
	: public IListenerGOG
	, public galaxy::api::GlobalLobbyListListener
	, public galaxy::api::GlobalLobbyDataRetrieveListener
{
public:

	FRequestLobbyListListener(const TSharedRef<FOnlineSessionSearch>& InOutSearchSettings);

private:

	void TriggerOnFindSessionsCompleteDelegates(bool InIsSuccessful) const;

	bool FillSearchResult(const galaxy::api::GalaxyID& InLobbyID, FOnlineSessionSearchResult* InSearchResult) const;

	void OnLobbyList(uint32_t InlobbyCount, bool InIOFailure) override;

	void OnLobbyDataRetrieveSuccess(const galaxy::api::GalaxyID& InLobbyID) override;

	void OnLobbyDataRetrieveFailure(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::ILobbyDataRetrieveListener::FailureReason InFailureReason) override;

	TArray<galaxy::api::GalaxyID> pendingLobbyList;
	TSharedRef<FOnlineSessionSearch> searchSettings;
};
