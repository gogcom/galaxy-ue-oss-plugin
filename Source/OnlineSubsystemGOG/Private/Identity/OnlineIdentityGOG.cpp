#include "OnlineIdentityGOG.h"

#include "OnlineSubsystemGOG.h"
#include "Loggers.h"
#include "Types/UniqueNetIdGOG.h"
#include "Types/UserOnlineAccountGOG.h"
#include "UserInfoUtils.h"

#include "OnlineError.h"

#include <array>

namespace
{

	bool GetRequireBackendAuthorization()
	{
		bool requireBackendAuthrorization{true};
		if (GConfig->GetBool(TEXT_CONFIG_SECTION_GOG, TEXT("bRequireBackendAuthorization"), requireBackendAuthrorization, GEngineIni))
			return requireBackendAuthrorization;

		return true;
	}

	constexpr auto MAX_STEAM_APP_TICKET_SIZE = 1024;

}

FOnlineIdentityGOG::FOnlineIdentityGOG(FOnlineSubsystemGOG& InOnlineSubsystemGOG)
	: onlineSubsystemGOG{InOnlineSubsystemGOG}
	, ownUserOnlineAccount{MakeShared<FUserOnlineAccountGOG>(FUniqueNetIdGOG{})}
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::ctor()"));
}

FOnlineIdentityGOG::~FOnlineIdentityGOG()
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::dtor()"));
}

bool FOnlineIdentityGOG::Login(int32 InLocalUserNum, const FOnlineAccountCredentials& InAccountCredentials)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::Login()"));

	if (isAuthInProgress)
	{
		UE_LOG_ONLINE_IDENTITY(Display, TEXT("Another authentication attempt is already inprogress"));
		return false;
	}

	CheckLocalUserNum(InLocalUserNum);

#if PLATFORM_WINDOWS || PLATFORM_MAC

	auto accountType = InAccountCredentials.Type.ToLower();

	if (accountType == TEXT("steam"))
	{
		UE_LOG_ONLINE_IDENTITY(Display, TEXT("Trying to log in as Steam user '%s'"), *InAccountCredentials.Id);

		std::array<uint8, MAX_STEAM_APP_TICKET_SIZE> steamAppTicketBuffer;

		const auto steamAppTicketSize = HexToBytes(InAccountCredentials.Token, steamAppTicketBuffer.data());

		galaxy::api::User()->SignInSteam(steamAppTicketBuffer.data(), steamAppTicketSize, TCHAR_TO_UTF8(*InAccountCredentials.Id));
		auto err = galaxy::api::GetError();
		if (err)
		{
			const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
			TriggerOnLoginCompleteDelegates(InLocalUserNum, false, *GetUniquePlayerId(InLocalUserNum), errorMessage);
			return false;
		}

		isAuthInProgress = true;
		return true;
	}

	if (accountType == TEXT("test"))
	{
		UE_LOG_ONLINE_IDENTITY(Display, TEXT("Trying to log in as user '%s'"), *InAccountCredentials.Id);
		galaxy::api::User()->SignInCredentials(TCHAR_TO_UTF8(*InAccountCredentials.Id), TCHAR_TO_UTF8(*InAccountCredentials.Token));
		auto err = galaxy::api::GetError();
		if (err)
		{
			const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
			TriggerOnLoginCompleteDelegates(InLocalUserNum, false, FUniqueNetIdGOG{}, errorMessage);
			return false;
		}

		isAuthInProgress = true;
		return true;
	}

	UE_LOG_ONLINE_IDENTITY(Display, TEXT("Trying to log in with Galaxy Client"));
	galaxy::api::User()->SignInGalaxy(GetRequireBackendAuthorization());
	auto err = galaxy::api::GetError();
	if (err)
	{
		const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
		TriggerOnLoginCompleteDelegates(InLocalUserNum, false, FUniqueNetIdGOG{}, errorMessage);
		return false;
	}

	isAuthInProgress = true;
	return true;

#elif PLATFORM_XBOXONE

	UE_LOG_ONLINE_IDENTITY(Display, TEXT("Trying to log in with XBOX ONE UserID '%s'"), *InAccountCredentials.Id);
	galaxy::api::User()->SignInXB1(TCHAR_TO_UTF8(*InAccountCredentials.Id));
	auto err = galaxy::api::GetError();
	if (err)
	{
		const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
		TriggerOnLoginCompleteDelegates(InLocalUserNum, false, FUniqueNetIdGOG{}, errorMessage);
		return false;
	}

	isAuthInProgress = true;
	return true;

#elif PLATFORM_PS4

	UE_LOG_ONLINE_IDENTITY(Display, TEXT("Trying to log in with PS4 ClientID '%s'"), *InAccountCredentials.Id);
	galaxy::api::User()->SignInPS4(TCHAR_TO_UTF8(*InAccountCredentials.Id), nullptr, nullptr, 0);
	auto err = galaxy::api::GetError();
	if (err)
	{
		const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
		TriggerOnLoginCompleteDelegates(InLocalUserNum, false, FUniqueNetIdGOG{}, errorMessage);
		return false;
	}

	isAuthInProgress = true;
	return true;

#else

	UE_LOG_ONLINE_IDENTITY(Error, TEXT("Invalid online account type: %s"), *InAccountCredentials.Type);
	return false;

#endif
}

bool FOnlineIdentityGOG::Logout(int32 InLocalUserNum)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::Logout()"));

	galaxy::api::User()->SignOut();
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to sign out; %s : %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return false;
	}

	isSigningOut = true;
	return true;
}

bool FOnlineIdentityGOG::AutoLogin(int32 InLocalUserNum)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::AutoLogin(%d)"), InLocalUserNum);

	// TODO: this has to be a dedicated server login
	Login(InLocalUserNum, {GetAuthType(), TEXT(""), TEXT("")});

	return true;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityGOG::GetUserAccount(const FUniqueNetId& InUserId) const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetUserAccount()"));

	if (InUserId == *ownUserOnlineAccount->GetUserId())
		return ownUserOnlineAccount;

	if (!UserInfoUtils::IsUserInfoAvailable(InUserId))
		return nullptr;

	return MakeShared<FUserOnlineAccountGOG>(InUserId);
}

TArray<TSharedPtr<FUserOnlineAccount>> FOnlineIdentityGOG::GetAllUserAccounts() const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetAllUserAccounts()"));

	return {ownUserOnlineAccount};
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::GetUniquePlayerId(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetUniquePlayerId()"));

	CheckLocalUserNum(InLocalUserNum);

	return ownUserOnlineAccount->GetUserId();
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::GetSponsorUniquePlayerId(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Error, TEXT("FOnlineIdentityGOG::GetSponsorUniquePlayerId() is not available on GOG platform"));

	return nullptr;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::CreateUniquePlayerId(uint8* InBytes, int32 InSize)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::CreateUniquePlayerId()"));

	auto newGalaxyID = MakeShared<FUniqueNetIdGOG>(InBytes, InSize);
	if (!newGalaxyID->IsValid())
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to create unique playerID: bytes=%u, size=%u"), InBytes, InSize);
		return nullptr;
	}

	if (galaxy::api::GalaxyID{*newGalaxyID}.GetIDType() != galaxy::api::GalaxyID::ID_TYPE_USER)
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Created PlayerID is not a UserID: bytes=%u, size=%u"), InBytes, InSize);
		return nullptr;
	}

	return newGalaxyID;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::CreateUniquePlayerId(const FString& InStr)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::CreateUniquePlayerId()"));

	auto newGalaxyID = MakeShared<FUniqueNetIdGOG>(InStr);
	if (!newGalaxyID->IsValid())
	{
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to create UniquePlayerId: inStr=%s"), *InStr);
		return nullptr;
	}

	return newGalaxyID;
}

ELoginStatus::Type FOnlineIdentityGOG::GetLoginStatus(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetLoginStatus()"));

	CheckLocalUserNum(InLocalUserNum);

	if (InLocalUserNum != LOCAL_USER_NUM)
		return ELoginStatus::NotLoggedIn;

	if (galaxy::api::User()->IsLoggedOn())
		return ELoginStatus::LoggedIn;

	if (galaxy::api::User()->SignedIn())
		return ELoginStatus::UsingLocalProfile;

	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityGOG::GetLoginStatus(const FUniqueNetId& InUserId) const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetLoginStatus()"));

	if(InUserId != *ownUserOnlineAccount->GetUserId())
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Only single local player is supported"));

	return GetLoginStatus(LOCAL_USER_NUM);
}

FString FOnlineIdentityGOG::GetPlayerNickname(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetPlayerNickname()"));

	CheckLocalUserNum(InLocalUserNum);

	return ownUserOnlineAccount->GetDisplayName();
}

FString FOnlineIdentityGOG::GetPlayerNickname(const FUniqueNetId& InUserId) const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetPlayerNickname()"));

	if (InUserId == *ownUserOnlineAccount->GetUserId())
		return ownUserOnlineAccount->GetDisplayName();

	FString playerNickname;
	if (!UserInfoUtils::GetPlayerNickname(InUserId, playerNickname))
		return{};

	return playerNickname;
}

FString FOnlineIdentityGOG::GetAuthToken(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetAuthToken()"));

	CheckLocalUserNum(InLocalUserNum);

	if (GetLoginStatus(InLocalUserNum) != ELoginStatus::LoggedIn)
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Trying to get auth toke for a user that is not logged in"));

	return ownUserOnlineAccount->GetAccessToken();
}

void FOnlineIdentityGOG::GetUserPrivilege(const FUniqueNetId& InUserId, EUserPrivileges::Type InPrivilege, const FOnGetUserPrivilegeCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetUserPrivilege()"));

	if (InUserId != *ownUserOnlineAccount->GetUserId())
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Only single local player is supported"));
		return;
	}

	auto privilegeResult = static_cast<uint32>(EPrivilegeResults::NoFailures);
	switch (InPrivilege)
	{
		case EUserPrivileges::CanCommunicateOnline:
			if (!galaxy::api::Chat())
			{
				privilegeResult |= static_cast<uint32>(EPrivilegeResults::ChatRestriction);
				break;
			}
		case EUserPrivileges::CanPlayOnline:
			if (!galaxy::api::User()->IsLoggedOn())
			{
				privilegeResult |= static_cast<uint32>(EPrivilegeResults::UserNotLoggedIn)
					| static_cast<uint32>(EPrivilegeResults::OnlinePlayRestricted);
				break;
			}
		case EUserPrivileges::CanPlay:
			// No restrictions from playing online or offline
			break;

		case EUserPrivileges::CanUseUserGeneratedContent:
			// No restrictions from using user generated content by other users
			break;

		default:
			UE_LOG_ONLINE_IDENTITY(Error, TEXT("Unknown privilege type"));
	}

	InDelegate.ExecuteIfBound(InUserId, InPrivilege, privilegeResult);
}

#if ENGINE_MINOR_VERSION >= 19
FPlatformUserId FOnlineIdentityGOG::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& InUniqueNetId) const
#else
FPlatformUserId FOnlineIdentityGOG::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& InUniqueNetId)
#endif
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetPlatformUserIdFromUniqueNetId()"));

	if(InUniqueNetId != *ownUserOnlineAccount->GetUserId())
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Only single local player is supported"));

	return LOCAL_USER_NUM;
}

#if ENGINE_MINOR_VERSION >= 19
void FOnlineIdentityGOG::RevokeAuthToken(const FUniqueNetId& InUserId, const FOnRevokeAuthTokenCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::RevokeAuthToken()"));

	if (InUserId != *ownUserOnlineAccount->GetUserId())
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Only single local player is supported"));
		InDelegate.ExecuteIfBound(InUserId, FOnlineError{false});
		return;
	}

	galaxy::api::User()->ReportInvalidAccessToken(TCHAR_TO_UTF8(*ownUserOnlineAccount->GetAccessToken()));
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE_IDENTITY(Error, TEXT("Failed to get access token: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		InDelegate.ExecuteIfBound(InUserId, FOnlineError{501});
	}

	ownUserOnlineAccount->GetAccessToken().Empty();
	InDelegate.ExecuteIfBound(InUserId, FOnlineError{true});
}
#endif

FString FOnlineIdentityGOG::GetAuthType() const
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("FOnlineIdentityGOG::GetAuthType()"));

	return TEXT("galaxy");
};

void FOnlineIdentityGOG::OnAuthSuccess()
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("GOGAuthListener::OnAuthSuccess()"));

	check(IsInGameThread());

	isAuthInProgress = false;

	// Update cached info keeping shared ref
	*ownUserOnlineAccount = FUserOnlineAccountGOG{UserInfoUtils::GetOwnUserID()};
	if (!FUserOnlineAccountGOG::FillOwn(*ownUserOnlineAccount))
		return;

	galaxy::api::User()->RequestUserData();
	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_ONLINE_IDENTITY(Warning, TEXT("Failed to request user data: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

	TriggerOnLoginChangedDelegates(LOCAL_USER_NUM);
	TriggerOnLoginCompleteDelegates(LOCAL_USER_NUM, true, *ownUserOnlineAccount->GetUserId(), TEXT(""));

	UE_LOG_ONLINE_IDENTITY(Display, TEXT("Successfully logged in: name=%s, userID=%s"), *ownUserOnlineAccount->GetDisplayName(), *ownUserOnlineAccount->GetUserId()->ToString());
}

void FOnlineIdentityGOG::OnAuthFailure(FailureReason failureReason)
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("GOGAuthListener::OnAuthFailure()"));

	isAuthInProgress = false;

	check(IsInGameThread());

	UE_LOG_ONLINE_IDENTITY(Error, TEXT("Failed to authenticate: %s"), *FailureReasonToFString(failureReason));

	TriggerOnLoginCompleteDelegates(LOCAL_USER_NUM, false, FUniqueNetIdGOG{}, FailureReasonToFString(failureReason));
}

void FOnlineIdentityGOG::OnAuthLost()
{
	UE_LOG_ONLINE_IDENTITY(Display, TEXT("GOGAuthListener::OnAuthLost()"));

	isAuthInProgress = false;

	check(IsInGameThread());

	UE_LOG_ONLINE_IDENTITY(Error, TEXT("Authentication lost"));

	TriggerOnLoginChangedDelegates(LOCAL_USER_NUM);
	if (isSigningOut)
	{
		isSigningOut = false;
		TriggerOnLogoutCompleteDelegates(LOCAL_USER_NUM, true);
	}
}

void FOnlineIdentityGOG::OnOperationalStateChanged(uint32_t /*InOperationalState*/)
{
	auto newLoginStatus = GetLoginStatus(LOCAL_USER_NUM);
	TriggerOnLoginStatusChangedDelegates(LOCAL_USER_NUM, loginStatus, newLoginStatus, *ownUserOnlineAccount->GetUserId());
	loginStatus = newLoginStatus;
}

FString FOnlineIdentityGOG::FailureReasonToFString(FailureReason failureReason)
{
	switch (failureReason)
	{
		case FAILURE_REASON_GALAXY_SERVICE_NOT_AVAILABLE:
			return TEXT("Galaxy Service is not installed properly or fails to start.");
		case FAILURE_REASON_GALAXY_SERVICE_NOT_SIGNED_IN:
			return TEXT("Galaxy Service is not signed in properly.");
		case FAILURE_REASON_CONNECTION_FAILURE:
			return TEXT("Unable to communicate with backend services.");
		case FAILURE_REASON_NO_LICENSE:
			return TEXT("User that is being signed in has no license for this application.");
		case FAILURE_REASON_INVALID_CREDENTIALS:
			return TEXT("Unable to match client credentials (ID, secret) or user credentials (username, password).");
		case FAILURE_REASON_GALAXY_NOT_INITIALIZED:
			return TEXT("Galaxy has not been initialized.");
		case FAILURE_REASON_UNDEFINED:
		default:
			return TEXT("Undefined error.");
	};
}

TSharedRef<FUserOnlineAccountGOG> FOnlineIdentityGOG::GetOwnUserOnlineAccount() const
{
	return ownUserOnlineAccount;
}
