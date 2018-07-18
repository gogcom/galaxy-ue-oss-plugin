#pragma once

#include "CommonGOG.h"
#include "CachedLeaderboardsDetails.h"
#include "Types/IListenerGOG.h"

#include "Interfaces/OnlineLeaderboardInterface.h"
#include "OnlineStats.h"

class FOnlineLeaderboardsGOG : public IOnlineLeaderboards
{
public:

	bool ReadLeaderboards(const TArray<TSharedRef<const FUniqueNetId>>& InPlayers, FOnlineLeaderboardReadRef& InOutReadLeaderboard) override;

	bool ReadLeaderboardsForFriends(int32 InLocalUserNum, FOnlineLeaderboardReadRef& InInOutReadLeaderboard) override;

	bool ReadLeaderboardsAroundRank(int32 InRank, uint32 InRange, FOnlineLeaderboardReadRef& InInOutReadLeaderboard) override;

	bool ReadLeaderboardsAroundUser(TSharedRef<const FUniqueNetId> InPlayerID, uint32 InRange, FOnlineLeaderboardReadRef& InInOutReadLeaderboard) override;

	void FreeStats(FOnlineLeaderboardRead& InInOutReadLeaderboard) override;

	bool WriteLeaderboards(const FName& InSessionName, const FUniqueNetId& InPlayer, FOnlineLeaderboardWrite& InWriteLeaderboard) override;

	bool FlushLeaderboards(const FName& InSessionName) override;

	bool WriteOnlinePlayerRatings(const FName& InSessionName, int32 leaderboardId, const TArray<FOnlinePlayerScore>& InPlayerScores) override;

PACKAGE_SCOPE:

	FOnlineLeaderboardsGOG(const class FOnlineSubsystemGOG& InOnlineSubsystemGOG);

	void RemoveCachedLeaderboard(FName InSessionName);

	// TBD: Introduce listener manager ("SDK-2232: Employ matchmaking specific listeners in UE plugin")
	void FreeListener(const FSetElementId& InListenerID)
	{
		listenerRegistry.Remove(InListenerID);
	}

	template<class Listener, typename... Args>
	FSetElementId CreateListener(Args&&... args)
	{
		auto listenerID = listenerRegistry.Add(MakeUnique<Listener>(Forward<Args>(args)...));
		listenerRegistry[listenerID]->ListenerID = listenerID;
		return listenerID;
	}

	template<class Listener>
	Listener* GetListenerRawPtr(FSetElementId InListenerID)
	{
		return dynamic_cast<Listener*>(listenerRegistry[InListenerID].Get());
	}

private:

	bool MarkLeaderboardStarted(FOnlineLeaderboardReadRef& InOutReadLeaderboard, IOnlineLeaderboards* onlineLeaderboardsInterface) const;

	bool UpdateWriteCache(const FName& InSessionName, FOnlineLeaderboardWrite &InWriteLeaderboard);

	TSet<TUniquePtr<IListenerGOG>> listenerRegistry;

	using SessionName = FName;
	using LeaderboardName = FName;
	TMap<const SessionName, TMap<const LeaderboardName, CachedLeaderboardDetails>> writeLeaderboardCache;

	const class FOnlineSubsystemGOG& onlineSubsystemGOG;
};
