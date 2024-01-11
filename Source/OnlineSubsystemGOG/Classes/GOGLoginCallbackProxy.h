#pragma once

#include "Interfaces/OnlineIdentityInterface.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "GameFramework/PlayerController.h"

#include "GOGLoginCallbackProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnlineLogoutResultDelegate, APlayerController*, PlayerController);

UCLASS(MinimalAPI)
class UGOGLoginCallbackProxy : public UOnlineBlueprintCallProxyBase
{

	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FOnlineLogoutResultDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOnlineLogoutResultDelegate OnFailure;

	/* Log in to platform backend services
	 *
	 * To log in with Galaxy Client, leave parameters empty. In this case, config parameter 'bRequireBackendAuthorization' is taken into consideration.
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Online")
	static UGOGLoginCallbackProxy* Login(UObject* WorldContextObject, class APlayerController* PlayerController, FString AuthType, FString UserID, FString UserToken);

	void Activate() override;

private:

	void OnLoginComplete(int32 InLocalUserNum, bool InWasSuccessful, const FUniqueNetId& InUserId, const FString& InErrorVal);

	bool UpdatePlayerUniqueNetID(int32 InLocalUserNum) const;

	IOnlineIdentityPtr onlineIdentity;

	FString authType;
	FString userID;
	FString userToken;

	FDelegateHandle loginCompleteDelegateHandle;

	UObject* worldContextObject;
	TWeakObjectPtr<APlayerController> playerControllerWeakPtr;
};
