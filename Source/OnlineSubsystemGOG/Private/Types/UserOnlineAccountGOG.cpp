#include "UserOnlineAccountGOG.h"

#include "CommonGOG.h"
#include <array>

namespace
{

	constexpr uint32_t MAX_ACCESSTOKEN_LENGHT = 1024;

	static FString GetOwnAccessToken()
	{
		std::array<char, MAX_ACCESSTOKEN_LENGHT> accessTokenBuffer;

		galaxy::api::User()->GetAccessTokenCopy(accessTokenBuffer.data(), accessTokenBuffer.size());
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get user access token: %s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return{};
		}

		return UTF8_TO_TCHAR(accessTokenBuffer.data());
	}

}

TSharedRef<FUserOnlineAccountGOG> FUserOnlineAccountGOG::FUserOnlineAccountGOG::CreateOwn()
{
	return MakeShared<FUserOnlineAccountGOG>(UserInfoUtils::GetOwnUserID(), UserInfoUtils::GetOwnPlayerNickname(), GetOwnAccessToken());
}

FUserOnlineAccountGOG::FUserOnlineAccountGOG(FUniqueNetIdGOG InUserID)
	: FUserOnlineAccountGOG{MoveTemp(InUserID), UserInfoUtils::GetPlayerNickname(InUserID)}
{
}

FUserOnlineAccountGOG::FUserOnlineAccountGOG(FUniqueNetIdGOG InUserID, FString InDisplayName, FString InAccessToken)
	: FOnlineUserGOG{MoveTemp(InUserID), MoveTemp(InDisplayName)}
	, accessToken{MoveTemp(InAccessToken)}
{
}

TSharedRef<const FUniqueNetId> FUserOnlineAccountGOG::GetUserId() const
{
	return FOnlineUserGOG::GetUserId();
}

FString FUserOnlineAccountGOG::GetRealName() const
{
	return FOnlineUserGOG::GetRealName();
}

FString FUserOnlineAccountGOG::GetDisplayName(const FString& InPlatform) const
{
	return FOnlineUserGOG::GetDisplayName(InPlatform);
}

bool FUserOnlineAccountGOG::GetUserAttribute(const FString& InAttrName, FString& OutAttrValue) const
{
	return FOnlineUserGOG::GetUserAttribute(InAttrName, OutAttrValue);
}

bool FUserOnlineAccountGOG::SetUserAttribute(const FString& InAttrName, const FString& InAttrValue)
{
	const auto previousValue = userAttributes.Find(InAttrName);
	if (!previousValue)
	{
		userAttributes.Emplace(InAttrName, InAttrValue);
		return true;
	}

	if (*previousValue == InAttrValue)
		return false;

	*previousValue = InAttrValue;
	return true;
}

FString FUserOnlineAccountGOG::GetAccessToken() const
{
	return accessToken;
}

bool FUserOnlineAccountGOG::GetAuthAttribute(const FString& /*InAttrName*/, FString& /*OutAttrValue*/) const
{
	UE_LOG_ONLINE(Warning, TEXT("GetAuthAttribute() is not available on GOG OnlineAccounts"));
	return false;
}
