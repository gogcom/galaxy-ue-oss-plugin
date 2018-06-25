#pragma once

#include "Types/IListenerGOG.h"
#include "Types/UniqueNetIdGOG.h"

#include "OnlineAchievementsInterface.h"

class FQueryAchievementsListener
	: public IListenerGOG
	, public galaxy::api::IUserStatsAndAchievementsRetrieveListener
{
public:

	FQueryAchievementsListener(const FUniqueNetIdGOG& InPlayerId, const FOnQueryAchievementsCompleteDelegate& InDelegate);

private:

	void OnUserStatsAndAchievementsRetrieveSuccess(galaxy::api::GalaxyID InUserID) override;

	void OnUserStatsAndAchievementsRetrieveFailure(galaxy::api::GalaxyID InUserID, galaxy::api::IUserStatsAndAchievementsRetrieveListener::FailureReason InFailureReason) override;

	void TriggerOnQueryAchievementsCompleteDelegate(bool InResult);

	const FUniqueNetIdGOG playerId;

	const FOnQueryAchievementsCompleteDelegate queryAchievementsCompleteDelegate;
};
