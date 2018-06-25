#include "WriteAchievementsListener.h"

#include "AchievementsInterfaceGOG.h"

#include "Online.h"

FWriteAchievementsListener::FWriteAchievementsListener(const FUniqueNetIdGOG& InPlayerId, FOnlineAchievementsWriteRef& InWriteObject, const FOnAchievementsWrittenDelegate& InDelegate)
	: playerId{InPlayerId}
	, achievementsWriteObject{InWriteObject}
	, achievementsWrittenDelegate{InDelegate}
{
}

void FWriteAchievementsListener::OnUserStatsAndAchievementsStoreSuccess()
{
	UE_LOG_ONLINE(Display, TEXT("OnUserStatsAndAchievementsStoreSuccess()"));

	TriggerOnAchievementsWrittenDelegate(true);
}

void FWriteAchievementsListener::OnUserStatsAndAchievementsStoreFailure(galaxy::api::IStatsAndAchievementsStoreListener::FailureReason InFailureReason)
{
	UE_LOG_ONLINE(Display, TEXT("OnUserStatsAndAchievementsStoreFailure"));

	TriggerOnAchievementsWrittenDelegate(false);
}

void FWriteAchievementsListener::TriggerOnAchievementsWrittenDelegate(bool InResult)
{
	achievementsWriteObject->WriteState = InResult
		? EOnlineAsyncTaskState::Done
		: EOnlineAsyncTaskState::Failed;

	achievementsWrittenDelegate.ExecuteIfBound(playerId, InResult);

	// Unlocked achivements are handled in FOnlineAchievementsGOG::OnAchievementUnlocked()

	auto onlineAchivementsInterface = StaticCastSharedPtr<FOnlineAchievementsGOG>(Online::GetAchievementsInterface());
	if (!onlineAchivementsInterface.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to clear write achivements listener. Possible memory leak: playerID='%s'"), *playerId.ToString());
		return;
	}

	onlineAchivementsInterface->FreeListener(ListenerID);
}