#include "UserOnlineAccountGOG.h"

FUserOnlineAccountGOG::FUserOnlineAccountGOG(FUniqueNetIdGOG InUserID, FString InDisplayName, FString InAccessToken)
	: userID{MakeShared<FUniqueNetIdGOG>(std::move(InUserID))}
	, displayName{std::move(InDisplayName)}
	, accessToken{std::move(InAccessToken)}
{
}

TSharedRef<const FUniqueNetId> FUserOnlineAccountGOG::GetUserId() const
{
	return userID;
}

FString FUserOnlineAccountGOG::GetRealName() const
{
	return{};
}

FString FUserOnlineAccountGOG::GetDisplayName(const FString& InPlatform) const
{
	return displayName;
}

bool FUserOnlineAccountGOG::GetUserAttribute(const FString& InAttrName, FString& OutAttrValue) const
{
	const auto foundAttr = userAttributes.Find(InAttrName);
	if (foundAttr == nullptr)
		return false;

	OutAttrValue = *foundAttr;
	return true;
}

FString FUserOnlineAccountGOG::GetAccessToken() const
{
	return accessToken;
}

bool FUserOnlineAccountGOG::GetAuthAttribute(const FString&, FString&) const
{
	UE_LOG_ONLINE(Warning, TEXT("GetAuthAttribute() is not available on GOG OnlineAccounts"));

	return false;
}

bool FUserOnlineAccountGOG::SetUserAttribute(const FString& InAttrName, const FString& InAttrValue)
{
	userAttributes.Add(InAttrName, InAttrValue);
	return true;
}
