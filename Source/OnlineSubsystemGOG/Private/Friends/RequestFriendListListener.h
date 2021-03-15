#pragma once

#include "Types/IListenerGOG.h"
#include "Types/UniqueNetIdGOG.h"

#include "Interfaces/OnlineFriendsInterface.h"

class FRequestFriendListListener
	: public IListenerGOG
	, public SelfDeregisteringListenerGOG<galaxy::api::IFriendListListener>
{
public:

	FRequestFriendListListener(class FOnlineFriendsGOG& InFriendsInterface, FString InListName, FOnReadFriendsListComplete InOnReadFriendsListCompleteDelegate);

private:

	void OnFriendListRetrieveSuccess() override;

	void OnFriendListRetrieveFailure(galaxy::api::IFriendListListener::FailureReason InFailureReason) override;

	void TriggerOnReadFriendsListCompleteDelegates(bool InWasSuccessful, FString InErrorMessage = FString{});

	class FOnlineFriendsGOG& friendsInterface;
	FString listName;
	FOnReadFriendsListComplete onReadFriendsListCompleteDelegate;
};
