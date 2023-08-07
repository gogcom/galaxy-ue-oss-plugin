#include "OnlineExternalUIGOG.h"

#include "Loggers.h"
#include "OnlineSubsystemGOG.h"
#include "Session/OnlineSessionGOG.h"
#include "Types/UserOnlineAccountGOG.h"
#include "Types/OnlineUserPresence.h"

FOnlineExternalUIGOG::FOnlineExternalUIGOG(const class FOnlineSubsystemGOG& InOnlineSubsystemGOG, TSharedRef<class FUserOnlineAccountGOG> InUserOnlineAccount)
	: onlineSubsystemGOG{InOnlineSubsystemGOG}
	, ownUserOnlineAccount{InUserOnlineAccount}
{
}

bool FOnlineExternalUIGOG::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, bool bShowSkipButton, const FOnLoginUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowLoginUI()"));
	
	return false;
}

bool FOnlineExternalUIGOG::ShowFriendsUI(int32 LocalUserNum)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowFriendsUI()"));
	
	return false;
}
	
bool FOnlineExternalUIGOG::ShowInviteUI(int32 LocalUserNum, FName SessionName)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowInviteUI()"));

	TSharedPtr<FOnlineSessionGOG, ESPMode::ThreadSafe> onlineSessionGOG{StaticCastSharedPtr<FOnlineSessionGOG>(onlineSubsystemGOG.GetSessionInterface())};
	if (!onlineSessionGOG.IsValid())
	{
		return false;
	}

	const FNamedOnlineSession* const Session = onlineSessionGOG->GetNamedSession(SessionName);
	if (Session && Session->SessionInfo.IsValid())
	{
		const FOnlineSessionInfoGOG* const SessionInfo = (FOnlineSessionInfoGOG*)(Session->SessionInfo.Get());
		if (SessionInfo->SessionID.IsValid())
		{
			char ConnectionStr[260];
			sprintf_s(ConnectionStr, sizeof(ConnectionStr), "+connect_lobby %s", TCHAR_TO_ANSI(*SessionInfo->SessionID.ToString()));
	
			UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("string=%s"), ANSI_TO_TCHAR(ConnectionStr));
			
			galaxy::api::Friends()->ShowOverlayInviteDialog(ConnectionStr);
		}

		return true;
	}

	return false;
}

bool FOnlineExternalUIGOG::ShowAchievementsUI(int32 LocalUserNum)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowAchievementsUI()"));
	
	return false;
}
	
bool FOnlineExternalUIGOG::ShowLeaderboardUI(const FString& LeaderboardName)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowLeaderboardUI()"));
	
	return false;
}

bool FOnlineExternalUIGOG::ShowWebURL(const FString& Url, const FShowWebUrlParams& ShowParams, const FOnShowWebUrlClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowWebURL()"));
	
	return false;
}

bool FOnlineExternalUIGOG::CloseWebURL()
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::CloseWebURL()"));
	
	return false;
}

bool FOnlineExternalUIGOG::ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate )
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowProfileUI()"));
	
	return false;
}

bool FOnlineExternalUIGOG::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowAccountUpgradeUI()"));
	
	return false;
}

bool FOnlineExternalUIGOG::ShowStoreUI(int32 LocalUserNum, const FShowStoreParams& ShowParams, const FOnShowStoreUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowStoreUI()"));
	
	return false;
}

bool FOnlineExternalUIGOG::ShowSendMessageUI(int32 LocalUserNum, const FShowSendMessageParams& ShowParams, const FOnShowSendMessageUIClosedDelegate& Delegate)
{
	UE_LOG_ONLINE_EXTERNALUI(Display, TEXT("FOnlineExternalUIGOG::ShowSendMessageUI()"));
	
	return false;
}
