#pragma once

#include "CommonGOG.h"
#include "UniqueNetIdGOG.h"

#include "OnlineSubsystemTypes.h"

class FUserOnlineAccountGOG : public FUserOnlineAccount
{
public:

	FUserOnlineAccountGOG(FUniqueNetIdGOG InUserID, FString InDisplayName, FString InAccessToken = {});

	// FOnlineUser

	TSharedRef<const FUniqueNetId> GetUserId() const override;

	FString GetRealName() const override;

	FString GetDisplayName(const FString& InPlatform) const override;

	bool GetUserAttribute(const FString& InAttrName, FString& OutAttrValue) const override;

	// FUserOnlineAccount

	FString GetAccessToken() const override;

	bool GetAuthAttribute(const FString& InAttrName, FString& OutAttrValue) const override;

	bool SetUserAttribute(const FString& InAttrName, const FString& AttrValue) override;

private:

	TSharedRef<const FUniqueNetId> userID;
	FString displayName;
	TMap<FString, FString> userAttributes;

	FString accessToken;
};
