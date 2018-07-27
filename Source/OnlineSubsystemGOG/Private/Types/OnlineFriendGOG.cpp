#include "OnlineFriendGOG.h"

#include "OnlinePresenceInterface.h"


FOnlineFriendGOG::FOnlineFriendGOG(FUniqueNetIdGOG InFriendID)
	: FOnlineUserGOG{MoveTemp(InFriendID)}
{
	// TODO: fill user presence once implemented https://jira.cdprojektred.com/browse/SDK-2270
}

TSharedRef<const FUniqueNetId> FOnlineFriendGOG::GetUserId() const
{
	return FOnlineUserGOG::GetUserId();
}

FString FOnlineFriendGOG::GetRealName() const
{
	return FOnlineUserGOG::GetRealName();
}

FString FOnlineFriendGOG::GetDisplayName(const FString& InPlatform) const
{
	return FOnlineUserGOG::GetDisplayName(InPlatform);
}

bool FOnlineFriendGOG::GetUserAttribute(const FString& InAttrName, FString& OutAttrValue) const
{
	return FOnlineUserGOG::GetUserAttribute(InAttrName, OutAttrValue);
}

EInviteStatus::Type FOnlineFriendGOG::GetInviteStatus() const
{
	return EInviteStatus::Accepted;
}

const FOnlineUserPresence& FOnlineFriendGOG::GetPresence() const
{
	return userPresence;
}

void FOnlineFriendGOG::SetPresence(FOnlineUserPresence InUserPresence)
{
	userPresence = MoveTemp(InUserPresence);
}
