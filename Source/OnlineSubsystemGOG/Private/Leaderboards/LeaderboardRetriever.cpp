#include "LeaderboardRetriever.h"

#include "Converters/OnlineLeaderboardConverter.h"
#include "OnlineLeaderboardsGOG.h"
#include "Types/UniqueNetIdGOG.h"

#include "Online.h"

#include <array>

namespace
{

	bool CreateAndFillLeaderboardEntry(
		const FOnlineLeaderboardReadRef& InOutReadLeaderboard,
		const galaxy::api::GalaxyID& InUserID,
		const uint32 InRank,
		const int32 InScore,
		const char* InDetailsBuffer,
		uint32_t InDetailsSize)
	{
		constexpr uint32_t MAX_USERNAME_LENGHT = 1024;
		std::array<char, MAX_USERNAME_LENGHT> usernameBuffer;

		galaxy::api::Friends()->GetFriendPersonaNameCopy(InUserID, usernameBuffer.data(), usernameBuffer.size());
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to read player name for retrieved leaderboard entry: leaderboardName='%s', playerId=%llu, %s; %s"),
				*InOutReadLeaderboard->LeaderboardName.ToString(), InUserID.ToUint64(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return false;
		}

		auto& newEntry = InOutReadLeaderboard->Rows.Emplace_GetRef(UTF8_TO_TCHAR(usernameBuffer.data()), MakeShared<FUniqueNetIdGOG>(InUserID));
		newEntry.Rank = InRank;

		newEntry.Columns.Add(InOutReadLeaderboard->SortedColumn, InScore);

		FString details{static_cast<int32>(InDetailsSize), UTF8_TO_TCHAR(InDetailsBuffer)};
		newEntry.Columns.Append(OnlineLeaderboardConverter::FromJsonString(MoveTemp(details)));

		return true;
	}

}

FLeaderboardRetriever::FLeaderboardRetriever(FOnlineLeaderboardReadRef InInOutReadLeaderboard)
	: readLeaderboard{MoveTemp(InInOutReadLeaderboard)}
{
	UE_LOG_ONLINE(Display, TEXT("Retrieving leaderboard: name=%s, sortedColumn=%s, readState=%s"),
		*readLeaderboard->LeaderboardName.ToString(),
		*readLeaderboard->SortedColumn.ToString(),
		EOnlineAsyncTaskState::ToString(readLeaderboard->ReadState));
}

void FLeaderboardRetriever::OnLeaderboardRetrieveSuccess(const char* InName)
{
	UE_LOG_ONLINE(Display, TEXT("OnLeaderboardRetrieveSuccess: leaderboardName='%s'"), UTF8_TO_TCHAR(InName));

	RequestLeaderboardEntries();
}

void FLeaderboardRetriever::OnLeaderboardRetrieveFailure(const char* InName, galaxy::api::ILeaderboardRetrieveListener::FailureReason failureReason)
{
	UE_LOG_ONLINE(Display, TEXT("OnLeaderboardRetrieveSuccess: leaderboardName='%s'"), UTF8_TO_TCHAR(InName));

	TriggerOnLeaderboardReadCompleteDelegates(false);
}

void FLeaderboardRetriever::OnLeaderboardEntriesRetrieveSuccess(const char* InName, uint32_t InEntryCount)
{
	UE_LOG_ONLINE(Display, TEXT("OnLeaderboardEntriesRetrieveSuccess: %s"), UTF8_TO_TCHAR(InName));

	uint32 rank;
	int32 score;
	galaxy::api::GalaxyID userID;
	// See galaxy::api::IStats::GetRequestedLeaderboardEntryWithDetails()
	constexpr uint32_t MAX_LEADERBOARD_DETAIL_SIZE = 3071;
	std::array<char, MAX_LEADERBOARD_DETAIL_SIZE> detailsBuffer;
	uint32_t actualDetailsSize;

	readLeaderboard->Rows.Empty();

	for (decltype(InEntryCount) idx{0}; idx < InEntryCount; ++idx)
	{
		galaxy::api::Stats()->GetRequestedLeaderboardEntryWithDetails(idx, rank, score, detailsBuffer.data(), detailsBuffer.size(), actualDetailsSize, userID);
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Error, TEXT("Failed to read retrieved leaderboard entries: leaderboardName='%s'; %s; %s"),
				*readLeaderboard->LeaderboardName.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

			TriggerOnLeaderboardReadCompleteDelegates(false);
			return;
		}

		if (!CreateAndFillLeaderboardEntry(readLeaderboard, userID, rank, score, detailsBuffer.data(), actualDetailsSize))
		{
			TriggerOnLeaderboardReadCompleteDelegates(false);
			return;
		}
	}

	TriggerOnLeaderboardReadCompleteDelegates(true);
}

void FLeaderboardRetriever::OnLeaderboardEntriesRetrieveFailure(const char* InName, galaxy::api::ILeaderboardEntriesRetrieveListener::FailureReason InFailureReason)
{
	UE_LOG_ONLINE(Display, TEXT("OnLeaderboardEntriesRetrieveFailure: leaderboardName='%s'"), UTF8_TO_TCHAR(InName));

	if (InFailureReason == galaxy::api::ILeaderboardEntriesRetrieveListener::FAILURE_REASON_NOT_FOUND)
	{
		UE_LOG_ONLINE(Error, TEXT("Could not find any entries for specified search criteria"));
	}
	else
	{
		UE_LOG_ONLINE(Error, TEXT("Could not retrieve leaderboard entries. Unknown failure"));
	}

	TriggerOnLeaderboardReadCompleteDelegates(false);
}

void FLeaderboardRetriever::TriggerOnLeaderboardReadCompleteDelegates(bool InResult)
{
	auto onlineLeaderboardInterface = StaticCastSharedPtr<FOnlineLeaderboardsGOG>(Online::GetLeaderboardsInterface());
	if (!onlineLeaderboardInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to clear ReadLeaderboard listener. Possible memory leak: leaderboardName='%s'"), *readLeaderboard->LeaderboardName.ToString());
		return;
	}

	readLeaderboard->ReadState = InResult
		? EOnlineAsyncTaskState::Done
		: EOnlineAsyncTaskState::Failed;

	onlineLeaderboardInterface->TriggerOnLeaderboardReadCompleteDelegates(InResult);

	onlineLeaderboardInterface->FreeListener(ListenerID);
}
