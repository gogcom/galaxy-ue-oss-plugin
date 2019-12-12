#include "AchievementsInterfaceGOG.h"

#include "Loggers.h"
#include "OnlineSubsystemGOG.h"
#include "QueryAchievementsListener.h"
#include "WriteAchievementsListener.h"

#include "Misc/ConfigCacheIni.h"
#include <array>

namespace
{

	constexpr double ACHIVEMENT_PROGRESS_LOCKED = 0.;
	constexpr double ACHIVEMENT_PROGRESS_UNLOCKED = 100.;

	constexpr uint32_t MAX_ACHIVEMENTS_BUFFER_SIZE = 4096;

}

FOnlineAchievementsGOG::FOnlineAchievementsGOG(FOnlineSubsystemGOG& InSubsystem, TSharedRef<FUserOnlineAccountGOG> InUserOnlineAccount)
	: subsystemGOG{InSubsystem}
	, ownUserOnlineAccount{MoveTemp(InUserOnlineAccount)}
{
	if (!GConfig->GetArray(TEXT_CONFIG_SECTION_GOG, TEXT_CONFIG_KEY_ACHIEVEMENTS, achievementIDs, GEngineIni))
		// Assert not declared achievements only when they are actually used (any methods is called)
		UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("Achievements declarations not found in Engine.ini configuration file"));
}

void FOnlineAchievementsGOG::WriteAchievements(const FUniqueNetId& InPlayerId, FOnlineAchievementsWriteRef& InWriteObject, const FOnAchievementsWrittenDelegate& InDelegate)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::WriteAchievements()"));

	if (!AssertAchievementsCount())
		return;

	if (*ownUserOnlineAccount->GetUserId() != InPlayerId)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Warning, TEXT("Achievements can be written only for self"));
		InWriteObject->WriteState = EOnlineAsyncTaskState::Failed;
		InDelegate.ExecuteIfBound(InPlayerId, false);
		return;
	}

	if (!InWriteObject->Properties.Num())
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("Empty achievements list"));
		InWriteObject->WriteState = EOnlineAsyncTaskState::Done;
		InDelegate.ExecuteIfBound(InPlayerId, true);
		return;
	}

	TArray<FOnlineAchievement> playerCachedAchievements;
	if (GetCachedAchievements(InPlayerId, playerCachedAchievements) == EOnlineCachedResult::NotFound)
	{
		InWriteObject->WriteState = EOnlineAsyncTaskState::Failed;
		InDelegate.ExecuteIfBound(InPlayerId, false);
		return;
	}

	InWriteObject->WriteState = EOnlineAsyncTaskState::InProgress;

	FOnlineAchievementDesc achievementDescription;
	for (const auto& achievement : InWriteObject->Properties)
	{
		if (GetCachedAchievementDescription(achievement.Key.ToString(), achievementDescription) == EOnlineCachedResult::NotFound)
		{
			UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Unknown achievement: achievementID='%s'"), *achievement.Key.ToString());
			InWriteObject->WriteState = EOnlineAsyncTaskState::Failed;
			InDelegate.ExecuteIfBound(InPlayerId, false);
			return;
		}

		// Ignore achievements progress value and consider all provided achievements as unlocked.
		// TBD: alternatively implement achievements progress as user stats (then ignore them when processing normal stats)
		// or create new Galaxy "progressable" achievements
		galaxy::api::Stats()->SetAchievement(TCHAR_TO_UTF8(*achievement.Key.ToString()));
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to unlock player achievement: playerID='%s'; achievementID='%s'; %s; %s"),
				*InPlayerId.ToString(), *achievement.Key.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

			InWriteObject->WriteState = EOnlineAsyncTaskState::Failed;
			InDelegate.ExecuteIfBound(InPlayerId, false);
			return;
		}
	}

	auto listener = CreateListener<FWriteAchievementsListener>(*this, InPlayerId, InWriteObject, InDelegate);

	galaxy::api::Stats()->StoreStatsAndAchievements(listener.Value);
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to store unlocked player achievements: playerID='%s'; %s; %s"),
			*InPlayerId.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		InWriteObject->WriteState = EOnlineAsyncTaskState::Failed;

		FreeListener(MoveTemp(listener.Key));

		InDelegate.ExecuteIfBound(InPlayerId, false);
		return;
	}
}

void FOnlineAchievementsGOG::QueryAchievements(const FUniqueNetId& InPlayerId, const FOnQueryAchievementsCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::QueryAchievements()"));

	if (!AssertAchievementsCount())
		return;

	auto listener = CreateListener<FQueryAchievementsListener>(*this, InPlayerId, InDelegate);

	galaxy::api::Stats()->RequestUserStatsAndAchievements(FUniqueNetIdGOG{InPlayerId}, listener.Value);
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to query player achievements: playerID='%s'; %s; %s"), *InPlayerId.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

		FreeListener(MoveTemp(listener.Key));

		InDelegate.ExecuteIfBound(InPlayerId, false);
	}
}

void FOnlineAchievementsGOG::QueryAchievementDescriptions(const FUniqueNetId& InPlayerId, const FOnQueryAchievementsCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::QueryAchievementDescriptions()"));

	if (!AssertAchievementsCount())
	{
		InDelegate.ExecuteIfBound(InPlayerId, false);
		return;
	}

	// Assuming achievements declarations and descriptions cannot be changed in a runtime, it's enough to request them only once

	if (!AreAchievementsDescriptionsAvailable())
	{
		QueryAchievements(InPlayerId, InDelegate);
		InDelegate.ExecuteIfBound(InPlayerId, false);
		return;
	}

	InDelegate.ExecuteIfBound(InPlayerId, true);
}

EOnlineCachedResult::Type FOnlineAchievementsGOG::GetCachedAchievement(const FUniqueNetId& InPlayerId, const FString& InAchievementId, FOnlineAchievement& OutAchievement)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::GetCachedAchievement()"));

	if (!AssertAchievementsCount())
		return EOnlineCachedResult::NotFound;

	TArray<FOnlineAchievement> playerCachedAchievements;
	if (GetCachedAchievements(InPlayerId, playerCachedAchievements) == EOnlineCachedResult::NotFound)
		return EOnlineCachedResult::NotFound;

	const auto cachedPlayerAchievement = playerCachedAchievements.FindByPredicate([&](const auto& playerAchievement)
	{
		return playerAchievement.Id == InAchievementId;
	});

	if (!cachedPlayerAchievement)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Specified achievement not found for the player: playerID='%s', achievementID='%s'"), *InPlayerId.ToString(), *InAchievementId);
		return EOnlineCachedResult::NotFound;
	}

	OutAchievement = *cachedPlayerAchievement;

	return EOnlineCachedResult::Success;;
};

EOnlineCachedResult::Type FOnlineAchievementsGOG::GetCachedAchievements(const FUniqueNetId& InPlayerId, TArray<FOnlineAchievement>& OutAchievements)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::GetCachedAchievements()"));

	if (!AssertAchievementsCount())
		return EOnlineCachedResult::NotFound;

	if (!cachedAchievements.Num())
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Achievements are not found. Please request achievements first with QueryAchievements"));
		return EOnlineCachedResult::NotFound;
	}

	const auto playerCachedAchievements = cachedAchievements.Find(InPlayerId);
	if (!playerCachedAchievements)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Achievements are not found for the player. Please request achievements first with QueryAchievements: playerID='%s'"), *InPlayerId.ToString());
		return EOnlineCachedResult::NotFound;
	}

	OutAchievements = *playerCachedAchievements;
	return EOnlineCachedResult::Success;
};

EOnlineCachedResult::Type FOnlineAchievementsGOG::GetCachedAchievementDescription(const FString& InAchievementId, FOnlineAchievementDesc& OutAchievementDesc)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::GetCachedAchievementDescription()"));

	if (!AssertAchievementsCount())
		return EOnlineCachedResult::NotFound;

	if (!AreAchievementsDescriptionsAvailable())
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Achievement descriptions are not found. Please request achievement descriptions first with QueryAchievementDescriptions: achievementId='%s'"), *InAchievementId);
		return EOnlineCachedResult::NotFound;
	}

	const auto achievementDescription = cachedAchievementDescriptions.Find(InAchievementId);
	if (!achievementDescription)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Description is not found for the specified achievement. Please request achievement descriptions first with QueryAchievementDescriptions: achievementId='%s'"), *InAchievementId);
		return EOnlineCachedResult::NotFound;
	}

	OutAchievementDesc = *achievementDescription;
	return EOnlineCachedResult::Success;
};

#if !UE_BUILD_SHIPPING
bool FOnlineAchievementsGOG::ResetAchievements(const FUniqueNetId& InPlayerId)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::ResetAchievements()"));

	if (!AssertAchievementsCount())
		return false;

	if (*ownUserOnlineAccount->GetUserId() != InPlayerId)
		return false;

	auto playerCachedAchievements = cachedAchievements.Find(InPlayerId);

	if (!playerCachedAchievements || playerCachedAchievements->Num() <= 0)
		return true;

	for (const auto& achievement : *playerCachedAchievements)
	{
		galaxy::api::Stats()->ClearAchievement(TCHAR_TO_UTF8(*achievement.Id));
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to reset player achievements: playerID='%s'; achievementID='%s'; %s; %s"),
				*InPlayerId.ToString(), *achievement.Id, UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

			continue;
		}
	}

	galaxy::api::Stats()->StoreStatsAndAchievements(this);
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to reset player achievements: playerID='%s'; %s; %s"), *InPlayerId.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return false;
	}

	playerCachedAchievements->Empty();

	areAchievementsReset = false;

	while (!areAchievementsReset)
		subsystemGOG.Tick(0.1);

	areAchievementsReset = false;
	return achievementsResetResult;
};

void FOnlineAchievementsGOG::OnUserStatsAndAchievementsStoreSuccess()
{
	areAchievementsReset = achievementsResetResult = true;
}

void FOnlineAchievementsGOG::OnUserStatsAndAchievementsStoreFailure(galaxy::api::IStatsAndAchievementsStoreListener::FailureReason failureReason)
{
	areAchievementsReset = true;
	achievementsResetResult = false;
}

#endif

auto FOnlineAchievementsGOG::AchievementsCount() const
{
	return achievementIDs.Num();
}

inline bool FOnlineAchievementsGOG::AssertAchievementsCount() const
{
	if (AchievementsCount() < 0)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Achievements declarations not found in Engine.ini configuration file. Please provide achievements declaration before using them."));
		return false;
	}

	return true;
}

bool FOnlineAchievementsGOG::AreAchievementsDescriptionsAvailable() const
{
	return cachedAchievementDescriptions.Num() != 0;
}

void FOnlineAchievementsGOG::OnAchievementUnlocked(const char* InName)
{
	UE_LOG_ONLINE_ACHIEVEMENTS(Display, TEXT("FOnlineAchievementsGOG::OnAchievementUnlocked()"));

	FString achievementName{UTF8_TO_TCHAR(InName)};

	auto ownUserID = ownUserOnlineAccount->GetUserId();
	if (!ownUserID->IsValid())
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Cannot unlock player achievement. Invalid local user ID: achievementName='%s'"), *achievementName);
		return;
	}

	auto playerCachedAchievements = cachedAchievements.Find(*ownUserID);
	if (!playerCachedAchievements)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Cannot unlock player achievement. Player achievements not found: achievementName='%s', playerID='%s'"),
			*achievementName, *ownUserID->ToString());
		return;
	}

	auto playerCachedAchievement = playerCachedAchievements->FindByPredicate([&](const auto& playerAchievement)
	{
		return playerAchievement.Id == UTF8_TO_TCHAR(InName);
	});

	if (!playerCachedAchievement)
	{
		UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Unlocked achievement not found. Achievements IDs missing in Engine.ini? : achievementKey='%s'"), *achievementName);
		return;
	}

	playerCachedAchievement->Progress = ACHIVEMENT_PROGRESS_UNLOCKED;
	TriggerOnAchievementUnlockedDelegates(*ownUserID, playerCachedAchievement->Id);
}

void FOnlineAchievementsGOG::OnAchievementsRetrieved(const FUniqueNetIdGOG& InPlayerID)
{
	UpdateAchievementDescriptions();
	AddOrReplacePlayerAchievements(InPlayerID);
}

void FOnlineAchievementsGOG::AddOrReplacePlayerAchievements(const FUniqueNetIdGOG& InPlayerID)
{
	// Emplace overriding any existing values
	auto& playerCachedAchievements = cachedAchievements.Emplace(InPlayerID);
	playerCachedAchievements.Reserve(AchievementsCount());

	bool isUnlocked;
	// Ignore UnlockTime as it is in FOnlineAchievementDesc instead on FOnlineAchievement. This is clearly a bug in UE
	uint32_t unlockTime;
	decltype(galaxy::api::GetError()) err;
	for (const auto& achievementID : achievementIDs)
	{
		galaxy::api::Stats()->GetAchievement(TCHAR_TO_UTF8(*achievementID), isUnlocked, unlockTime, InPlayerID);
		playerCachedAchievements.Emplace(FOnlineAchievement{achievementID, isUnlocked ? ACHIVEMENT_PROGRESS_UNLOCKED : ACHIVEMENT_PROGRESS_LOCKED});
		err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to read achievement for player: achievementID='%s'; playerID='%s'; %s; %s"),
				*achievementID, *InPlayerID.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return;
		}
	}
}

bool FOnlineAchievementsGOG::UpdateAchievementDescriptions()
{
	// Assuming list of achievements cannot change in a runtime, update descriptions only once
	if (AreAchievementsDescriptionsAvailable())
		return true;

	cachedAchievementDescriptions.Reserve(AchievementsCount());

	std::array<char, MAX_ACHIVEMENTS_BUFFER_SIZE> achievementInfoBuffer;

	FString title;
	FString description;
	bool isHidden;

	for (const auto& achievementID : achievementIDs)
	{
		const auto* achievementIDAsUTF8 = TCHAR_TO_UTF8(*achievementID);

		galaxy::api::Stats()->GetAchievementDisplayNameCopy(achievementIDAsUTF8, achievementInfoBuffer.data(), achievementInfoBuffer.size());
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to read achievement title: achievementID='%s'; %s; %s"), *achievementID, UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return false;
		}
		title = UTF8_TO_TCHAR(achievementInfoBuffer.data());

		galaxy::api::Stats()->GetAchievementDescriptionCopy(achievementIDAsUTF8, achievementInfoBuffer.data(), achievementInfoBuffer.size());
		err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to read achievement description: achievementID='%s'; %s; %s"), *achievementID, UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return false;
		}
		description = UTF8_TO_TCHAR(achievementInfoBuffer.data());

		isHidden = !galaxy::api::Stats()->IsAchievementVisible(achievementIDAsUTF8);
		err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE_ACHIEVEMENTS(Error, TEXT("Failed to get achievement visibility: achievementID='%s'; %s; %s"), *achievementID, UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return false;
		}

		cachedAchievementDescriptions.Emplace(achievementID, FOnlineAchievementDesc{FText::FromString(title), FText::FromString(description), FText::FromString(description), isHidden});
	}

	return true;
}
