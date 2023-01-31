#include "GOGLoginCallbackProxy.h"

#include "Online.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "OnlineSubsystemUtils.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GameFramework/PlayerState.h"

UGOGLoginCallbackProxy::UGOGLoginCallbackProxy(const FObjectInitializer& ObjectInitializer)
	: Super{ObjectInitializer}
{}

UGOGLoginCallbackProxy* UGOGLoginCallbackProxy::Login(UObject* InWorldContextObject, class APlayerController* InPlayerController, FString AuthType, FString InUserID, FString InUserToken)
{
#if ENGINE_MINOR_VERSION >= 17 || ENGINE_MAJOR_VERSION > 4
	auto onlineSubsystem = Online::GetSubsystem(GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::ReturnNull));
#else
	auto onlineSubsystem = Online::GetSubsystem(GEngine->GetWorldFromContextObject(InWorldContextObject, true));
#endif
	if (onlineSubsystem == nullptr)
	{
		FFrame::KismetExecutionMessage(*FString::Printf(TEXT("Login - Invalid or uninitialized OnlineSubsystem"), "Login"), ELogVerbosity::Warning);
		return nullptr;
	}

	auto onlineIdentity = onlineSubsystem->GetIdentityInterface();
	if (!onlineIdentity.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Login - OnlineIdentity interface is not available for current platform."), ELogVerbosity::Error);
		return nullptr;
	}

	auto callbackProxy = NewObject<UGOGLoginCallbackProxy>();
	callbackProxy->worldContextObject = InWorldContextObject;
	callbackProxy->playerControllerWeakPtr = InPlayerController;
	callbackProxy->authType = AuthType;
	callbackProxy->userID = InUserID;
	callbackProxy->userToken = InUserToken;

	callbackProxy->onlineIdentity = onlineIdentity;

	return callbackProxy;
}

void UGOGLoginCallbackProxy::Activate()
{
	if (!playerControllerWeakPtr.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Login failed: invalid PlayerController."), ELogVerbosity::Error);
		OnFailure.Broadcast(playerControllerWeakPtr.Get());
		return;
	}

	if (!onlineIdentity.IsValid())
		return;

	auto* localPlayer = Cast<ULocalPlayer>(playerControllerWeakPtr->Player);
	if (!localPlayer)
	{
		FFrame::KismetExecutionMessage(TEXT("Login failed: invalid Player object for given PlayerController."), ELogVerbosity::Error);
		OnFailure.Broadcast(playerControllerWeakPtr.Get());
		return;
	}

	const auto playerControllerId = localPlayer->GetControllerId();
	if (playerControllerId == INVALID_CONTROLLERID)
	{
		FFrame::KismetExecutionMessage(TEXT("Login failed: invalid PlayerControllerID."), ELogVerbosity::Error);
		OnFailure.Broadcast(playerControllerWeakPtr.Get());
		return;
	}

	if (onlineIdentity->OnLoginCompleteDelegates[playerControllerId].IsBoundToObject(this))
	{
		FFrame::KismetExecutionMessage(TEXT("Login failed: another authentication process is already in progress."), ELogVerbosity::Error);
		return;
	}

	loginCompleteDelegateHandle = onlineIdentity->AddOnLoginCompleteDelegate_Handle(playerControllerId, FOnLoginCompleteDelegate::CreateUObject(this, &ThisClass::OnLoginComplete));
	if (!onlineIdentity->Login(playerControllerId, {authType.IsEmpty() ? onlineIdentity->GetAuthType() : authType, userID, userToken}))
	{
		FFrame::KismetExecutionMessage(TEXT("Login failed: invalid OnlineIdentityInterface."), ELogVerbosity::Error);
		OnFailure.Broadcast(playerControllerWeakPtr.Get());
		return;
	}
}

void UGOGLoginCallbackProxy::OnLoginComplete(int32 InLocalUserNum, bool InWasSuccessful, const FUniqueNetId& /*InUserId*/, const FString& InErrorVal)
{
	if (onlineIdentity.IsValid())
		onlineIdentity->ClearOnLogoutCompleteDelegate_Handle(InLocalUserNum, loginCompleteDelegateHandle);

	if (InWasSuccessful && UpdatePlayerUniqueNetID(InLocalUserNum))
		OnSuccess.Broadcast(playerControllerWeakPtr.Get());
	else
		OnFailure.Broadcast(playerControllerWeakPtr.Get());
}

bool UGOGLoginCallbackProxy::UpdatePlayerUniqueNetID(int32 InLocalUserNum) const
{
	if (!onlineIdentity.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot update Player UniqueNetID: invalid OnlineIdentityInterface."), ELogVerbosity::Error);
		return false;
	}

	if (!playerControllerWeakPtr.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot update Player UniqueNetID: invalid PlayerController."), ELogVerbosity::Error);
		return false;
	}
#if ENGINE_MAJOR_VERSION > 4
	TObjectPtr<APlayerState> playerState = playerControllerWeakPtr->PlayerState;
#else
	auto* playerState = playerControllerWeakPtr->PlayerState;
#endif

	if (!playerState)
	{
		FFrame::KismetExecutionMessage(TEXT("Cannot update Player UniqueNetID: invalid PlayerState object for given PlayerController."), ELogVerbosity::Error);
		return false;
	}
#if ENGINE_MAJOR_VERSION > 4
	playerState->SetUniqueId(FUniqueNetIdRepl(onlineIdentity->GetUniquePlayerId(InLocalUserNum)));
#else
	playerState->SetUniqueId(onlineIdentity->GetUniquePlayerId(InLocalUserNum));
#endif
	return true;
}