#pragma once

#include "Types/IListenerGOG.h"

#include "Types/UniqueNetIdGOG.h"

#include "OnlineAchievementsInterface.h"

class FWriteAchievementsListener
	: public IListenerGOG
	, public galaxy::api::IStatsAndAchievementsStoreListener
{
public:

	FWriteAchievementsListener(const FUniqueNetIdGOG& InPlayerId, FOnlineAchievementsWriteRef& InWriteObject, const FOnAchievementsWrittenDelegate& InDelegate);

private:

	void OnUserStatsAndAchievementsStoreSuccess() override;

	void OnUserStatsAndAchievementsStoreFailure(galaxy::api::IStatsAndAchievementsStoreListener::FailureReason InFailureReason) override;

	void TriggerOnAchievementsWrittenDelegate(bool InResult);

	const FUniqueNetIdGOG playerId;
	FOnlineAchievementsWriteRef achievementsWriteObject;
	const FOnAchievementsWrittenDelegate achievementsWrittenDelegate;
};
