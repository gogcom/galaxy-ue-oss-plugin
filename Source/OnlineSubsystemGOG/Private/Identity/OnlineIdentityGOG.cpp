#include "OnlineIdentityGOG.h"

#include "OnlineSubsystemGOG.h"
#include "Types/UniqueNetIdGOG.h"
#include "Types/UserOnlineAccountGOG.h"
#include "OnlineError.h"

#include <array>

FOnlineIdentityGOG::FOnlineIdentityGOG(FOnlineSubsystemGOG& InOnlineSubsystemGOG)
	: onlineSubsystemGOG{InOnlineSubsystemGOG}
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::ctor()"));
}

FOnlineIdentityGOG::~FOnlineIdentityGOG()
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::dtor()"));
}

bool FOnlineIdentityGOG::Login(int32 InLocalUserNum, const FOnlineAccountCredentials& InAccountCredentials)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::Login()"));

	if (isAuthInProgress)
	{
		UE_LOG_ONLINE(Display, TEXT("Another authentication attempt is already inprogress"));
		return false;
	}

	CheckLocalUserNum(InLocalUserNum);

#if PLATFORM_WINDOWS

	auto accountType = InAccountCredentials.Type.ToLower();

	if (accountType == TEXT("steam"))
	{
		UE_LOG_ONLINE(Display, TEXT("Trying to log in as Steam user '%s'"), *InAccountCredentials.Id);
		galaxy::api::User()->SignIn(TCHAR_TO_UTF8(*InAccountCredentials.Token), CharLen(InAccountCredentials.Token), TCHAR_TO_UTF8(*InAccountCredentials.Id));
		auto err = galaxy::api::GetError();
		if (err)
		{
			const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			UE_LOG_ONLINE(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
			TriggerOnLoginCompleteDelegates(InLocalUserNum, false, *GetUniquePlayerId(InLocalUserNum), errorMessage);
			return false;
		}

		isAuthInProgress = true;
		return true;
	}

	if (accountType == TEXT("test"))
	{
		UE_LOG_ONLINE(Display, TEXT("Trying to log in as user '%s'"), *InAccountCredentials.Id);
		galaxy::api::User()->SignIn(TCHAR_TO_UTF8(*InAccountCredentials.Id), TCHAR_TO_UTF8(*InAccountCredentials.Token));
		auto err = galaxy::api::GetError();
		if (err)
		{
			const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			UE_LOG_ONLINE(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
			TriggerOnLoginCompleteDelegates(InLocalUserNum, false, *GetUniquePlayerId(InLocalUserNum), errorMessage);
			return false;
		}

		isAuthInProgress = true;
		return true;
	}

	UE_LOG_ONLINE(Display, TEXT("Trying to log in with Galaxy Client"));
	galaxy::api::User()->SignIn(true);
	auto err = galaxy::api::GetError();
	if (err)
	{
		const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		UE_LOG_ONLINE(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
		TriggerOnLoginCompleteDelegates(InLocalUserNum, false, *GetUniquePlayerId(InLocalUserNum), errorMessage);
		return false;
	}

	isAuthInProgress = true;
	return true;

#elif PLATFORM_XBOXONE

	UE_LOG_ONLINE(Display, TEXT("Trying to log in with XBOX ONE UserID '%s'"), *InAccountCredentials.Id);
	galaxy::api::User()->SignIn(static_cast<uint32_t>(FCString::Atoi(*InAccountCredentials.Id)));
	auto err = galaxy::api::GetError();
	if (err)
	{
		const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		UE_LOG_ONLINE(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
		TriggerOnLoginCompleteDelegates(InLocalUserNum, false, *GetUniquePlayerId(InLocalUserNum), errorMessage);
		return false;
	}

	isAuthInProgress = true;
	return true;

#elif PLATFORM_PS4

	UE_LOG_ONLINE(Display, TEXT("Trying to log in with PS4 ClientID '%s'"), *InAccountCredentials.Id);
	galaxy::api::User()->SignIn(TCHAR_TO_UTF8(*InAccountCredentials.Id), nullptr, nullptr, 0);
	auto err = galaxy::api::GetError();
	if (err)
	{
		const auto& errorMessage = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		UE_LOG_ONLINE(Warning, TEXT("Failed to sign in: %s"), *errorMessage);
		TriggerOnLoginCompleteDelegates(InLocalUserNum, false, *GetUniquePlayerId(InLocalUserNum), errorMessage);
		return false;
	}

	isAuthInProgress = true;
	return true;

#else

	UE_LOG_ONLINE(Error, TEXT("Invalid online account type: %s"), *InAccountCredentials.Type);
	return false;

#endif
}

bool FOnlineIdentityGOG::Logout(int32 InLocalUserNum)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::Logout()"));

	UE_LOG_ONLINE(Error, TEXT("'Logout()' operation is not available on GOG platform"));

	TriggerOnLoginChangedDelegates(InLocalUserNum);
	TriggerOnLogoutCompleteDelegates(InLocalUserNum, false);
	return false;
}

bool FOnlineIdentityGOG::AutoLogin(int32 InLocalUserNum)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::AutoLogin(%d)"), InLocalUserNum);

	// TODO: this has to be a dedicated server login
	Login(InLocalUserNum, {GetAuthType(), TEXT(""), TEXT("")});

	return true;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityGOG::GetUserAccount(const FUniqueNetId& InUserId) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetUserAccount()"));

	auto userInfo = CreateUserInfo(InUserId);

	if (!userInfo.IsValid())
		return userInfo;

	FillUserData(userInfo);

	return userInfo;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityGOG::CreateUserInfo(const FUniqueNetId &InUserId) const
{
	const auto galaxyID = galaxy::api::GalaxyID(AsUniqueNetIDGOG(InUserId));

	if (!galaxy::api::Friends()->IsUserInformationAvailable(galaxyID))
	{
		UE_LOG_ONLINE(Warning, TEXT("No user information available: userID='%s'"), *InUserId.ToString());
		return nullptr;
	}

	auto displayName = GetPlayerNickname(InUserId);
	if (displayName.IsEmpty())
		return nullptr;

	auto userInfo = MakeShared<FUserOnlineAccountGOG>(
		InUserId,
		std::move(displayName),
		onlineSubsystemGOG.IsLocalPlayer(InUserId) ? GetAuthToken(LOCAL_USER_NUM) : FString{});

	{
		auto personaState = galaxy::api::Friends()->GetFriendPersonaState(galaxyID);
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get persona state: userID='%s'; %s; %s"), *InUserId.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		}
		else
		{
			constexpr auto PERSONAME_STATE_KEY = TEXT("persona_state");
			constexpr auto PERSONAME_STATE_ONLINE = TEXT("online");
			constexpr auto PERSONAME_STATE_OFFLINE = TEXT("offline");

			userInfo->SetUserAttribute(PERSONAME_STATE_KEY,
				personaState == galaxy::api::PERSONA_STATE_ONLINE
				? PERSONAME_STATE_ONLINE
				: PERSONAME_STATE_OFFLINE);
		}
	}

	{
		constexpr uint32_t MAX_AVATAR_URL_LENGHT = 2048;
		std::array<char, MAX_AVATAR_URL_LENGHT> avatarUrlBuffer;

		galaxy::api::Friends()->GetFriendAvatarUrlCopy(galaxyID, galaxy::api::AVATAR_TYPE_LARGE, avatarUrlBuffer.data(), avatarUrlBuffer.size());
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get avatar url: userID='%s'; %s; %s"), *InUserId.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		}
		else
		{
			constexpr auto AVATAR_LARGE_KEY = TEXT("avatar_large");

			userInfo->SetUserAttribute(AVATAR_LARGE_KEY, UTF8_TO_TCHAR(avatarUrlBuffer.data()));
		}
	}

	return userInfo;
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityGOG::FillUserData(TSharedPtr<FUserOnlineAccount> InUserInfo) const
{
	const auto galaxyID = galaxy::api::GalaxyID(AsUniqueNetIDGOG(*InUserInfo->GetUserId()));

	if (!galaxy::api::User()->IsUserDataAvailable(galaxyID))
	{
		UE_LOG_ONLINE(Display, TEXT("No user data available: userID='%s'"), *InUserInfo->GetUserId()->ToString());
		return InUserInfo;
	}

	auto userDataCount = galaxy::api::User()->GetUserDataCount(galaxyID);
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to get user data count: userID='%s'; %s; %s"), *InUserInfo->GetUserId()->ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return InUserInfo;
	}

	constexpr uint32_t MAX_USERDATA_KEY_LENGHT = 1024;
	constexpr uint32_t MAX_USERDATA_VALUE_LENGHT = 1024;
	std::array<char, MAX_USERDATA_KEY_LENGHT> keyBuffer;
	std::array<char, MAX_USERDATA_VALUE_LENGHT> valueBuffer;

	for (uint32_t i = 0; i < userDataCount; ++i)
	{
		galaxy::api::User()->GetUserDataByIndex(i, keyBuffer.data(), keyBuffer.size(), valueBuffer.data(), valueBuffer.size(), galaxyID);
		err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to get user name: userID='%s'; %s; %s"), *InUserInfo->GetUserId()->ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return InUserInfo;
		}

		InUserInfo->SetUserAttribute(UTF8_TO_TCHAR(keyBuffer.data()), UTF8_TO_TCHAR(valueBuffer.data()));
	}

	return InUserInfo;
}

TArray<TSharedPtr<FUserOnlineAccount>> FOnlineIdentityGOG::GetAllUserAccounts() const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetAllUserAccounts()"));

	return {GetUserAccount(*GetUniquePlayerId(LOCAL_USER_NUM))};
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::GetUniquePlayerId(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetUniquePlayerId()"));

	CheckLocalUserNum(InLocalUserNum);

	auto userID = galaxy::api::User()->GetGalaxyID();
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to get user ID: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return nullptr;
	}

	if (!userID.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to get user ID"));
		return nullptr;
	}

	return MakeShared<FUniqueNetIdGOG>(userID);
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::GetSponsorUniquePlayerId(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE(Error, TEXT("FOnlineIdentityGOG::GetSponsorUniquePlayerId() is not available on GOG platform"));

	return nullptr;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::CreateUniquePlayerId(uint8* InBytes, int32 InSize)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::CreateUniquePlayerId()"));

	auto newGalaxyID = MakeShared<FUniqueNetIdGOG>(InBytes, InSize);
	if (!newGalaxyID->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to create unique playerID: bytes=%u, size=%u"), InBytes, InSize);
		return nullptr;
	}

	return newGalaxyID;
}

TSharedPtr<const FUniqueNetId> FOnlineIdentityGOG::CreateUniquePlayerId(const FString& InStr)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::CreateUniquePlayerId()"));

	auto newGalaxyID = MakeShared<FUniqueNetIdGOG>(InStr);
	if (!newGalaxyID->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to create UniquePlayerId: inStr=%s"), *InStr);
		return nullptr;
	}

	return newGalaxyID;
}

ELoginStatus::Type FOnlineIdentityGOG::GetLoginStatus(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetLoginStatus()"));

	CheckLocalUserNum(InLocalUserNum);

	if (InLocalUserNum != LOCAL_USER_NUM)
		return ELoginStatus::NotLoggedIn;

	if (galaxy::api::User()->IsLoggedOn())
		return ELoginStatus::LoggedIn;

	if (galaxy::api::User()->SignedIn())
		return ELoginStatus::UsingLocalProfile;

	return ELoginStatus::NotLoggedIn;
}

ELoginStatus::Type FOnlineIdentityGOG::GetLoginStatus(const FUniqueNetId& userId) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetLoginStatus()"));

	check(userId == *GetUniquePlayerId(LOCAL_USER_NUM) && "Only single local user is supported by GOG platform");

	return GetLoginStatus(LOCAL_USER_NUM);
}

FString FOnlineIdentityGOG::GetPlayerNickname(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetPlayerNickname()"));

	if (InLocalUserNum != LOCAL_USER_NUM)
	{
		UE_LOG_ONLINE(Warning, TEXT("Only single local player is supported"));
		return FString(TEXT(""));
	}

	constexpr uint32_t MAX_USERNAME_LENGHT = 1024;
	std::array<char, MAX_USERNAME_LENGHT> usernameBuffer;

	galaxy::api::Friends()->GetPersonaNameCopy(usernameBuffer.data(), usernameBuffer.size());
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to get players user name: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return {};
	}

	return FString{UTF8_TO_TCHAR(usernameBuffer.data())};
}

FString FOnlineIdentityGOG::GetPlayerNickname(const FUniqueNetId& InUserId) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetPlayerNickname()"));

	constexpr uint32_t MAX_USERNAME_LENGHT = 1024;
	std::array<char, MAX_USERNAME_LENGHT> usernameBuffer;

	galaxy::api::Friends()->GetFriendPersonaNameCopy(AsUniqueNetIDGOG(InUserId), usernameBuffer.data(), usernameBuffer.size());
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to get user name: userID='%s'; %s; %s"), *InUserId.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return {};
	}

	return FString{UTF8_TO_TCHAR(usernameBuffer.data())};
}

FString FOnlineIdentityGOG::GetAuthToken(int32 InLocalUserNum) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetAuthToken()"));

	CheckLocalUserNum(InLocalUserNum);

	if (GetLoginStatus(InLocalUserNum) != ELoginStatus::LoggedIn)
		UE_LOG_ONLINE(Warning, TEXT("Trying to get auth toke for a user that is not logged in"));

	constexpr uint32_t MAX_ACCESSTOKEN_LENGHT = 1024;
	std::array<char, MAX_ACCESSTOKEN_LENGHT> accessTokenBuffer;

	galaxy::api::User()->GetAccessTokenCopy(accessTokenBuffer.data(), accessTokenBuffer.size());
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to get user access token: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return {};
	}


	return FString(UTF8_TO_TCHAR(accessTokenBuffer.data()));
}

void FOnlineIdentityGOG::GetUserPrivilege(const FUniqueNetId& InUserId, EUserPrivileges::Type InPrivilege, const FOnGetUserPrivilegeCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetUserPrivilege()"));

	check(InUserId == *GetUniquePlayerId(LOCAL_USER_NUM) && "Only single local user is supported by GOG platform");

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
			check(false && "Unknown privilege type");
	}

	InDelegate.ExecuteIfBound(InUserId, InPrivilege, privilegeResult);
}

FPlatformUserId FOnlineIdentityGOG::GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& InUniqueNetId) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetPlatformUserIdFromUniqueNetId()"));

	check(InUniqueNetId == *GetUniquePlayerId(LOCAL_USER_NUM) && "Only single local user is supported by GOG platform");

	return LOCAL_USER_NUM;
}

FString FOnlineIdentityGOG::GetAuthType() const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::GetAuthType()"));

	return "galaxy";
};

void FOnlineIdentityGOG::RevokeAuthToken(const FUniqueNetId& InUserId, const FOnRevokeAuthTokenCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineIdentityGOG::RevokeAuthToken()"));

	check(InUserId == *GetUniquePlayerId(LOCAL_USER_NUM) && "Only single local user is supported by GOG platform");

	auto accessToken = GetAuthToken(LOCAL_USER_NUM);
	if (accessToken.IsEmpty())
	{
		UE_LOG_ONLINE(Display, TEXT("Trying to revoke empty access token"));
		return;
	}

	galaxy::api::User()->ReportInvalidAccessToken(TCHAR_TO_UTF8(*accessToken));
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to get access token: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		InDelegate.ExecuteIfBound(InUserId, FOnlineError{501});
	}

	InDelegate.ExecuteIfBound(InUserId, FOnlineError{true});
}

void FOnlineIdentityGOG::OnAuthSuccess()
{
	UE_LOG_ONLINE(Display, TEXT("GOGAuthListener::OnAuthSuccess()"));

	check(IsInGameThread());

	isAuthInProgress = false;

	galaxy::api::User()->RequestUserData();
	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_ONLINE(Warning, TEXT("Failed to request user data: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

	auto userID = GetUniquePlayerId(LOCAL_USER_NUM);

	TriggerOnLoginChangedDelegates(LOCAL_USER_NUM);
	TriggerOnLoginCompleteDelegates(LOCAL_USER_NUM, true, *userID, TEXT(""));

	UE_LOG_ONLINE(Display, TEXT("Successfully logged in: name=%s, userID=%s"), *GetPlayerNickname(LOCAL_USER_NUM), *userID->ToString());
}

void FOnlineIdentityGOG::OnAuthFailure(FailureReason failureReason)
{
	UE_LOG_ONLINE(Display, TEXT("GOGAuthListener::OnAuthFailure()"));

	isAuthInProgress = false;

	check(IsInGameThread());

	UE_LOG_ONLINE(Error, TEXT("Failed to authenticate: %s"), *FailureReasonToFString(failureReason));

	TriggerOnLoginCompleteDelegates(LOCAL_USER_NUM, false, FUniqueNetIdGOG{}, FailureReasonToFString(failureReason));
}

void FOnlineIdentityGOG::OnAuthLost()
{
	UE_LOG_ONLINE(Display, TEXT("GOGAuthListener::OnAuthLost()"));

	isAuthInProgress = false;

	check(IsInGameThread());

	UE_LOG_ONLINE(Error, TEXT("Authentication lost"));

	TriggerOnLoginStatusChangedDelegates(LOCAL_USER_NUM, ELoginStatus::LoggedIn, ELoginStatus::NotLoggedIn, *GetUniquePlayerId(LOCAL_USER_NUM));
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
