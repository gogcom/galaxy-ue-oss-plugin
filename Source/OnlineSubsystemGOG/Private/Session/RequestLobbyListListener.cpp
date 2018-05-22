#include "RequestLobbyListListener.h"

#include "OnlineSessionGOG.h"
#include "LobbyData.h"

#include "Online.h"

#include <algorithm>
#include <array>

FRequestLobbyListListener::FRequestLobbyListListener(const TSharedRef<FOnlineSessionSearch>& InOutSearchSettings)
	: searchSettings{InOutSearchSettings}
{
	check(IsInGameThread());
}

void FRequestLobbyListListener::TriggerOnFindSessionsCompleteDelegates(bool InIsSuccessful) const
{
	searchSettings->SearchState = InIsSuccessful
		? EOnlineAsyncTaskState::Done
		: EOnlineAsyncTaskState::Failed;

	auto onlineSessionInterface = StaticCastSharedPtr<FOnlineSessionGOG>(Online::GetSessionInterface());
	if (!onlineSessionInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to finalize session searching as OnlineSession interface is invalid"));
		return;
	}

	onlineSessionInterface->TriggerOnFindSessionsCompleteDelegates(InIsSuccessful);

	onlineSessionInterface->FreeListener(ListenerID);
}

void FRequestLobbyListListener::OnLobbyList(uint32_t InLobbyCount, bool InIOFailure)
{
	UE_LOG_ONLINE(Display, TEXT("FRequestLobbyListListener::OnLobbyList()"));

	if (InIOFailure)
	{
		UE_LOG_ONLINE(Error, TEXT("Unknown I/O failure while retrieving lobby list"));

		TriggerOnFindSessionsCompleteDelegates(false);
		return;
	}

	if (InLobbyCount == 0)
	{
		UE_LOG_ONLINE(Log, TEXT("Empty lobby list"));
		searchSettings->SearchResults.Empty();
		TriggerOnFindSessionsCompleteDelegates(true);
		return;
	}

	InLobbyCount = std::min(InLobbyCount, static_cast<uint32_t>(searchSettings->MaxSearchResults));

	for (decltype(InLobbyCount) lobbbyIdx = 0; lobbbyIdx < InLobbyCount; ++lobbbyIdx)
	{
		auto lobbyID = galaxy::api::Matchmaking()->GetLobbyByIndex(lobbbyIdx);
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get lobby[%d]: %s; %s"), lobbbyIdx, ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));
			continue;
		}

		// TODO: we probably have to use AsyncTaskManager to track lobby data requests nicely, e.g. timeouts
		if (!galaxy::api::Matchmaking()->RequestLobbyData(lobbyID))
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to request lobby data: %llu"), lobbyID.ToUint64());
			continue;
		}
		err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to request lobby data: %llu; %s; %s"), lobbbyIdx, ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));
			continue;
		}
		pendingLobbyList.Add(lobbyID);
	}

	if (!pendingLobbyList.Num())
	{
		UE_LOG_ONLINE(Warning, TEXT("Lobby list retrieved, yet cannot request data for any lobby"));

		TriggerOnFindSessionsCompleteDelegates(false);
		return;
	}

	searchSettings->SearchResults.Empty(pendingLobbyList.Num());

	UE_LOG_ONLINE(Display, TEXT("Waiting for lobby data to be retrieved"));
}

bool FRequestLobbyListListener::FillSearchResult(const galaxy::api::GalaxyID& InLobbyID, FOnlineSessionSearchResult* InSearchResult) const
{
	//TODO: Fill session info
	return true;
}

void FRequestLobbyListListener::OnLobbyDataRetrieveSuccess(const galaxy::api::GalaxyID& InLobbyID)
{
	UE_LOG_ONLINE(Display, TEXT("FRequestLobbyListListener::OnLobbyDataRetrieveSuccess()"), InLobbyID.ToUint64());

	if (pendingLobbyList.Find(InLobbyID) == INDEX_NONE)
	{
		UE_LOG_ONLINE(Display, TEXT("Unknown lobby. Skipping: lobbyID=%llu"), InLobbyID.ToUint64());
		return;
	}
	pendingLobbyList.RemoveSwap(InLobbyID);

	auto* newSearchResult = new (searchSettings->SearchResults) FOnlineSessionSearchResult();
	if (!newSearchResult)
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to create new OnlineSessionSearchResult. Null object"));
		return;
	}

	if (!FillSearchResult(InLobbyID, newSearchResult))
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to fill Session data: lobbyID=%llu"), InLobbyID.ToUint64());
		TriggerOnFindSessionsCompleteDelegates(false);
		return;
	}

	if (pendingLobbyList.Num() == 0)
		TriggerOnFindSessionsCompleteDelegates(true);
}

void FRequestLobbyListListener::OnLobbyDataRetrieveFailure(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::ILobbyDataRetrieveListener::FailureReason InFailureReason)
{
	UE_LOG_ONLINE(Display, TEXT("OnLobbyDataRetrieveFailure, lobbyID=%llu"), InLobbyID.ToUint64());

	if (pendingLobbyList.Find(InLobbyID) == INDEX_NONE)
	{
		UE_LOG_ONLINE(Display, TEXT("Unknown lobby. Skipping: lobbyID=%llu"), InLobbyID.ToUint64());
		return;
	}

	switch (InFailureReason)
	{
		case galaxy::api::ILobbyDataRetrieveListener::FAILURE_REASON_LOBBY_DOES_NOT_EXIST:
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to get lobby data. Lobby does not exists: lobbyID=%llu"), InLobbyID.ToUint64());
			pendingLobbyList.Remove(InLobbyID);
			// Continue waiting for other lobbies data
			return;
		}
		case galaxy::api::ILobbyDataRetrieveListener::FAILURE_REASON_UNDEFINED:
		default:
			UE_LOG_ONLINE(Error, TEXT("Lobby does not exists: lobbyID=%llu"), InLobbyID.ToUint64());
	}

	TriggerOnFindSessionsCompleteDelegates(false);
}
