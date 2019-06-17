#include "ReadLeaderboardAroundRankListener.h"
#include "OnlineLeaderboardsGOG.h"

#include <limits>
#include <algorithm>

#pragma warning (disable: 4868)
FReadLeaderboardAroundRankListener::FReadLeaderboardAroundRankListener(
	class FOnlineLeaderboardsGOG& InLeaderboardsInterface,
	int32 InRank,
	uint32 InRange,
	FOnlineLeaderboardReadRef InOutReadLeaderboard)
	: FLeaderboardRetriever{InLeaderboardsInterface, MoveTemp(InOutReadLeaderboard)}
	, rank{InRank}
	, range{InRange}
{
	check(rank > 0 && "Rank cannot be negative");
}
#pragma warning (default: 4868)

void FReadLeaderboardAroundRankListener::RequestLeaderboardEntries()
{
	galaxy::api::Stats()->RequestLeaderboardEntriesGlobal(
		TCHAR_TO_UTF8(*readLeaderboard->LeaderboardName.ToString()),
		rank - std::min<uint32>(rank, range),
		static_cast<uint32>(std::min<uint64>(std::numeric_limits<uint32>::max(), rank + range)),
		this);

	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE_LEADERBOARD(Error, TEXT("Failed to request leaderboard entries around rank: leaderboardName='%s', rank=%d, range=%u; %s; %s"),
			*readLeaderboard->LeaderboardName.ToString(), rank, range, UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

		TriggerOnLeaderboardReadCompleteDelegates(false);
		return;
	}
}
