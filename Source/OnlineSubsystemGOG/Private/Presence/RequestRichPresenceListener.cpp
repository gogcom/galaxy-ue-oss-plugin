#include "RequestRichPresenceListener.h"

#include "OnlinePresenceGOG.h"

FRequestRichPresenceListener::FRequestRichPresenceListener(FOnlinePresenceGOG& InPresenceInterface, FUniqueNetIdGOG InUserID, IOnlinePresence::FOnPresenceTaskCompleteDelegate InDelegate)
	: presenceInterface{InPresenceInterface}
	, userID{MoveTemp(InUserID)}
	, onPresenceRetrieveCompleteDelegate{MoveTemp(InDelegate)}
{
}

void FRequestRichPresenceListener::OnRichPresenceRetrieveSuccess(galaxy::api::GalaxyID InUserID)
{
	UE_LOG_ONLINE_PRESENCE(Display, TEXT("FRequestRichPresenceListener: OnRichPresenceRetrieveSuccess()"));
	TriggerOnRichPresenceRetrieveCompleteDelegate(true);
}

void FRequestRichPresenceListener::OnRichPresenceRetrieveFailure(galaxy::api::GalaxyID InUserID, galaxy::api::IRichPresenceRetrieveListener::FailureReason /*InFailureReason*/)
{
	UE_LOG_ONLINE_PRESENCE(Warning, TEXT("FRequestRichPresenceListener::OnRichPresenceRetrieveFailure()"));
	TriggerOnRichPresenceRetrieveCompleteDelegate(false);
}

void FRequestRichPresenceListener::TriggerOnRichPresenceRetrieveCompleteDelegate(bool InWasSuccessful)
{
	onPresenceRetrieveCompleteDelegate.ExecuteIfBound(MoveTemp(userID), InWasSuccessful);
	presenceInterface.FreeListener(MoveTemp(ListenerID));
}
