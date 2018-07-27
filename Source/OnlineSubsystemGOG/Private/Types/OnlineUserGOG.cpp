#include "OnlineUserGOG.h"

FOnlineUserGOG::FOnlineUserGOG(FUniqueNetIdGOG InUserID)
	: userID{MakeShared<const FUniqueNetIdGOG>(MoveTemp(InUserID))}
	, displayName{UserInfoUtils::GetPlayerNickname(*userID)}
	, userAttributes{UserInfoUtils::GetUserAttributes(*userID)}
{
}

FOnlineUserGOG::FOnlineUserGOG(FUniqueNetIdGOG InUserID, FString InDisplayName)
	: userID{MakeShared<const FUniqueNetIdGOG>(MoveTemp(InUserID))}
	, displayName{MoveTemp(InDisplayName)}
	, userAttributes{UserInfoUtils::GetUserAttributes(*userID)}
{
}

TSharedRef<const FUniqueNetId> FOnlineUserGOG::GetUserId() const
{
	return userID;
}

FString FOnlineUserGOG::GetRealName() const
{
	return{};
}

FString FOnlineUserGOG::GetDisplayName(const FString& InPlatform /*= FString()*/) const
{
	return displayName;
}

bool FOnlineUserGOG::GetUserAttribute(const FString& InAttrName, FString& OutAttrValue) const
{
	const auto foundAttr = userAttributes.Find(InAttrName);
	if (!foundAttr)
		return false;

	OutAttrValue = *foundAttr;
	return true;
}

