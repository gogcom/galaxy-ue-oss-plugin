#pragma once

#include "UniqueNetIdGOG.h"
#include "UserInfoUtils.h"

#include "OnlineSubsystemTypes.h"

class FOnlineUserGOG : public FOnlineUser
{
public:

	TSharedRef<const FUniqueNetId> GetUserId() const override;

	FString GetRealName() const override;

	FString GetDisplayName(const FString& InPlatform = FString()) const override;

	bool GetUserAttribute(const FString& InAttrName, FString& OutAttrValue) const override;

protected:

	FOnlineUserGOG(FUniqueNetIdGOG InUserID);

	FOnlineUserGOG(FUniqueNetIdGOG InUserID, FString InDisplayName);

	FOnlineUserGOG() = delete;

	TSharedRef<const FUniqueNetIdGOG> userID;
	FString displayName;
	UserAttributesMap userAttributes;
};
