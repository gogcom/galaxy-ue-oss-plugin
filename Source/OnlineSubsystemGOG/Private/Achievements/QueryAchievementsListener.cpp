#include "QueryAchievementsListener.h"

#include "AchievementsInterfaceGOG.h"

#include "Online.h"

FQueryAchievementsListener::FQueryAchievementsListener(const FUniqueNetIdGOG& InPlayerId, const FOnQueryAchievementsCompleteDelegate& InDelegate)
	: playerId{InPlayerId}
	, queryAchievementsCompleteDelegate{InDelegate}
{
}

void FQueryAchievementsListener::OnUserStatsAndAchievementsRetrieveSuccess(galaxy::api::GalaxyID InUserID)
{
	UE_LOG_ONLINE(Display, TEXT("OnUserStatsAndAchievementsRetrieveSuccess: userID=%llu"), InUserID.ToUint64());

	check(InUserID == playerId && "Achievements retrieved for unknown user. This shall not be happening with Galaxy specific listeners");

	auto onlineAchivementsInterface = StaticCastSharedPtr<FOnlineAchievementsGOG>(Online::GetAchievementsInterface());
	if (!onlineAchivementsInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed update player achivements. OnlineAchievements interface is NULL: playerID='%s'"), *playerId.ToString());
		TriggerOnQueryAchievementsCompleteDelegate(false);
		return;
	}

	onlineAchivementsInterface->OnAchivementsRetrieved(InUserID);

	TriggerOnQueryAchievementsCompleteDelegate(true);
}

void FQueryAchievementsListener::OnUserStatsAndAchievementsRetrieveFailure(galaxy::api::GalaxyID InUserID, galaxy::api::IUserStatsAndAchievementsRetrieveListener::FailureReason)
{
	UE_LOG_ONLINE(Display, TEXT("OnUserStatsAndAchievementsRetrieveFailure: userID=%llu"), InUserID.ToUint64());

	check(InUserID == playerId && "Achievements retrieved for unknown user. This shall not be happening with Galaxy specific listeners");

	TriggerOnQueryAchievementsCompleteDelegate(false);
}

void FQueryAchievementsListener::TriggerOnQueryAchievementsCompleteDelegate(bool InResult)
{
	queryAchievementsCompleteDelegate.ExecuteIfBound(playerId, InResult);

	auto onlineAchivementsInterface = StaticCastSharedPtr<FOnlineAchievementsGOG>(Online::GetAchievementsInterface());
	if (!onlineAchivementsInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to clear query achivements listener. Possible memory leak: playerID='%s'"), *playerId.ToString());
		return;
	}

	onlineAchivementsInterface->FreeListener(ListenerID);
}