// By courtesy of Zeus
//
//
//
#pragma once

#include "CommonGOG.h"
#include "ListenerManager.h"

#include "Interfaces/OnlineExternalUIInterface.h"
#include "Types/UniqueNetIdGOG.h"
#include "OnlineSubsystemGOGPackage.h"

class FOnlineExternalUIGOG
	: public IOnlineExternalUI
	, public FListenerManager
{
public:

	bool ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate = FOnLoginUIClosedDelegate()) override;
	
	bool ShowAccountCreationUI(const int ControllerIndex, const FOnAccountCreationUIClosedDelegate& Delegate = FOnAccountCreationUIClosedDelegate()) override { /** NYI */ return false; }
	
	bool ShowFriendsUI(int32 LocalUserNum) override;
	
	bool ShowInviteUI(int32 LocalUserNum, FName SessionName = NAME_GameSession) override;
	
	bool ShowAchievementsUI(int32 LocalUserNum) override;
	
	bool ShowLeaderboardUI(const FString& LeaderboardName) override;
	
	bool ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate = FOnShowWebUrlClosedDelegate()) override;
	
	bool CloseWebURL() override;
	
	bool ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate = FOnProfileUIClosedDelegate()) override;
	
	bool ShowAccountUpgradeUI(const FUniqueNetId& UniqueId) override;
	
	bool ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate = FOnShowStoreUIClosedDelegate()) override;
	
	bool ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate = FOnShowSendMessageUIClosedDelegate()) override;

PACKAGE_SCOPE:

	FOnlineExternalUIGOG(const class FOnlineSubsystemGOG& InOnlineSubsystemGOG, TSharedRef<class FUserOnlineAccountGOG> InUserOnlineAccount);

private:

	FOnlineExternalUIGOG() = delete;

	const class FOnlineSubsystemGOG& onlineSubsystemGOG;
	TSharedRef<class FUserOnlineAccountGOG> ownUserOnlineAccount;
};
