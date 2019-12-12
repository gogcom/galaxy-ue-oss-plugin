#include "WriteAchievementsListener.h"

#include "AchievementsInterfaceGOG.h"

#include "Online.h"

FWriteAchievementsListener::FWriteAchievementsListener(
	class FOnlineAchievementsGOG& InAchievementsInterface,
	const FUniqueNetIdGOG& InPlayerId,
	FOnlineAchievementsWriteRef& InWriteObject,
	const FOnAchievementsWrittenDelegate& InDelegate)
	: achievementsInterface{InAchievementsInterface}
	, playerId{InPlayerId}
	, achievementsWriteObject{InWriteObject}
	, achievementsWrittenDelegate{InDelegate}
{
}

void FWriteAchievementsListener::OnUserStatsAndAchievementsStoreSuccess()
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("OnUserStatsAndAchievementsStoreSuccess()"));

	TriggerOnAchievementsWrittenDelegate(true);
}

void FWriteAchievementsListener::OnUserStatsAndAchievementsStoreFailure(galaxy::api::IStatsAndAchievementsStoreListener::FailureReason InFailureReason)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("OnUserStatsAndAchievementsStoreFailure"));

	TriggerOnAchievementsWrittenDelegate(false);
}

void FWriteAchievementsListener::TriggerOnAchievementsWrittenDelegate(bool InResult)
{
	achievementsWriteObject->WriteState = InResult
		? EOnlineAsyncTaskState::Done
		: EOnlineAsyncTaskState::Failed;

	achievementsWrittenDelegate.ExecuteIfBound(playerId, InResult);
	achievementsInterface.FreeListener(MoveTemp(ListenerID));

	// Unlocked achievements are handled in FOnlineAchievementsGOG::OnAchievementUnlocked()
}