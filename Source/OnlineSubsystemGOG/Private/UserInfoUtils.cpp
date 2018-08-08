#include "UserInfoUtils.h"

#include <array>

namespace UserInfoUtils
{

	namespace
	{

		constexpr uint32_t MAX_USERDATA_KEY_LENGHT = 1024;
		constexpr uint32_t MAX_USERDATA_VALUE_LENGHT = 1024;

		constexpr uint32_t MAX_AVATAR_URL_LENGHT = 2048;

		void GetUserData(const galaxy::api::GalaxyID& InUserID, UserAttributesMap& InUserAttributes)
		{
			if (!galaxy::api::User()->IsUserDataAvailable(InUserID))
				return;

			auto userDataCount = galaxy::api::User()->GetUserDataCount(InUserID);
			auto err = galaxy::api::GetError();
			if (err)
			{
				UE_LOG_ONLINE(Warning, TEXT("Failed to get user data count: userID=%llu; %s: %s"), InUserID.ToUint64(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
				return;
			}

			InUserAttributes.Reserve(userDataCount);

			std::array<char, MAX_USERDATA_KEY_LENGHT> keyBuffer;
			std::array<char, MAX_USERDATA_VALUE_LENGHT> valueBuffer;

			for (decltype(userDataCount) i = 0; i < userDataCount; ++i)
			{
				galaxy::api::User()->GetUserDataByIndex(i, keyBuffer.data(), keyBuffer.size(), valueBuffer.data(), valueBuffer.size(), InUserID);
				err = galaxy::api::GetError();
				if (err)
				{
					UE_LOG_ONLINE(Warning, TEXT("Failed to get user name: userID=%llu; %s: %s"), InUserID.ToUint64(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
					return;
				}

				InUserAttributes.Emplace(UTF8_TO_TCHAR(keyBuffer.data()), UTF8_TO_TCHAR(valueBuffer.data()));
			}
		}

	}

	FString GetPlayerAvatarUrl(const galaxy::api::GalaxyID& InUserID, galaxy::api::AvatarType InAvatarType)
	{
		std::array<char, MAX_AVATAR_URL_LENGHT> avatarUrlBuffer;

		galaxy::api::Friends()->GetFriendAvatarUrlCopy(InUserID, InAvatarType, avatarUrlBuffer.data(), avatarUrlBuffer.size());
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get avatar url: userID='%llu'; %s: %s"),
				InUserID.ToUint64(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return{};
		}

		return  UTF8_TO_TCHAR(avatarUrlBuffer.data());
	}

	FString GetOwnPlayerNickname()
	{
		std::array<char, MAX_USERNAME_LENGHT> usernameBuffer;

		galaxy::api::Friends()->GetPersonaNameCopy(usernameBuffer.data(), usernameBuffer.size());
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get players user name: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return{};
		}

		return FString{UTF8_TO_TCHAR(usernameBuffer.data())};
	}

	FString GetPlayerNickname(const FUniqueNetIdGOG& InUserId)
	{
		if (!InUserId.IsValid())
		{
			UE_LOG_ONLINE(Warning, TEXT("Invalid UserID"));
			return {};
		}

		std::array<char, MAX_USERNAME_LENGHT> usernameBuffer;

		galaxy::api::Friends()->GetFriendPersonaNameCopy(InUserId, usernameBuffer.data(), usernameBuffer.size());
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get user name: userID='%s'; %s: %s"),
				*InUserId.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return{};
		}

		return UTF8_TO_TCHAR(usernameBuffer.data());
	}

	bool IsUserInfoAvailable(const FUniqueNetIdGOG& InUserId)
	{
		if (!InUserId.IsValid())
		{
			UE_LOG_ONLINE(Warning, TEXT("Invalid UserID"));
			return false;
		}

		return galaxy::api::Friends()->IsUserInformationAvailable(InUserId);
	}

	FUniqueNetIdGOG GetOwnUserID()
	{
		auto galaxyID = galaxy::api::User()->GetGalaxyID();
		auto err = galaxy::api::GetError();
		if (err)
			UE_LOG_ONLINE(Warning, TEXT("Failed to get Galaxy UserID: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

		return galaxyID;
	}

	UserAttributesMap GetUserAttributes(const FUniqueNetIdGOG& InUserId)
	{
		if (!InUserId.IsValid())
		{
			UE_LOG_ONLINE(Warning, TEXT("Invalid UserID"));
			return{};
		}

		UserAttributesMap userAttributes;
		userAttributes.Emplace(AVATAR_SMALL_KEY, GetPlayerAvatarUrl(InUserId, galaxy::api::AVATAR_TYPE_SMALL));
		userAttributes.Emplace(AVATAR_MEDIUM_KEY, GetPlayerAvatarUrl(InUserId, galaxy::api::AVATAR_TYPE_MEDIUM));
		userAttributes.Emplace(AVATAR_LARGE_KEY, GetPlayerAvatarUrl(InUserId, galaxy::api::AVATAR_TYPE_LARGE));

		GetUserData(InUserId, userAttributes);

		return userAttributes;
	}

}
