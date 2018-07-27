#pragma once

#include "Types/UniqueNetIdGOG.h"

using UserAttributesMap = TMap<FString, FString>;

constexpr auto AVATAR_LARGE_KEY = TEXT("avatar_large");
constexpr auto AVATAR_MEDIUM_KEY = TEXT("avatar_medium");
constexpr auto AVATAR_SMALL_KEY = TEXT("avatar_small");

namespace UserInfoUtils
{

	FString GetOwnPlayerNickname();

	FString GetPlayerNickname(const FUniqueNetIdGOG& InUserId);

	FUniqueNetIdGOG GetOwnUserID();

	bool IsUserInfoAvailable(const FUniqueNetIdGOG& InUserId);

	UserAttributesMap GetUserAttributes(const FUniqueNetIdGOG& InUserId);

}