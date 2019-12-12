#pragma once

#include "Types/IListenerGOG.h"

#include "Types/UniqueNetIdGOG.h"

#include "Interfaces/OnlineAchievementsInterface.h"

class FWriteAchievementsListener
	: public IListenerGOG
	, public galaxy::api::IStatsAndAchievementsStoreListener
{
public:

	FWriteAchievementsListener(
		class FOnlineAchievementsGOG& InAchievementsInterface,
		const FUniqueNetIdGOG& InPlayerId,
		FOnlineAchievementsWriteRef& InWriteObject,
		const FOnAchievementsWrittenDelegate& InDelegate);

private:

	void OnUserStatsAndAchievementsStoreSuccess() override;

	void OnUserStatsAndAchievementsStoreFailure(galaxy::api::IStatsAndAchievementsStoreListener::FailureReason InFailureReason) override;

	void TriggerOnAchievementsWrittenDelegate(bool InResult);

	class FOnlineAchievementsGOG& achievementsInterface;
	const FUniqueNetIdGOG playerId;
	FOnlineAchievementsWriteRef achievementsWriteObject;
	const FOnAchievementsWrittenDelegate achievementsWrittenDelegate;
};
