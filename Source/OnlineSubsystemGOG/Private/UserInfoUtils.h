#pragma once

#include "Types/UniqueNetIdGOG.h"

using UserAttributesMap = TMap<FString, FString>;

constexpr uint32_t MAX_USERNAME_LENGHT = 1024;

constexpr auto AVATAR_LARGE_KEY = TEXT("avatar_large");
constexpr auto AVATAR_MEDIUM_KEY = TEXT("avatar_medium");
constexpr auto AVATAR_SMALL_KEY = TEXT("avatar_small");

namespace UserInfoUtils
{

	FString GetPlayerAvatarUrl(const galaxy::api::GalaxyID& InUserID, galaxy::api::AvatarType InAvatarType);

	FString GetOwnPlayerNickname();

	FString GetPlayerNickname(const FUniqueNetIdGOG& InUserId);

	FUniqueNetIdGOG GetOwnUserID();

	bool IsUserInfoAvailable(const FUniqueNetIdGOG& InUserId);

	UserAttributesMap GetUserAttributes(const FUniqueNetIdGOG& InUserId);

}