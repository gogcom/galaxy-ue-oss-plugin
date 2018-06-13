#include "OnlineSessionGOG.h"

#include "LobbyData.h"
#include "CreateLobbyListener.h"
#include "RequestLobbyListListener.h"
#include "JoinLobbyListener.h"
#include "LobbyStartListener.h"
#include "Network/InternetAddrGOG.h"

#include "Online.h"
#include "OnlineSubsystemUtils.h"

#include <algorithm>

FOnlineSessionGOG::FOnlineSessionGOG(IOnlineSubsystem& InSubsystem)
	: subsystemGOG{InSubsystem}
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::ctor()"));
}

FOnlineSessionGOG::~FOnlineSessionGOG()
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::dtor()"));

	for (const auto& session : storedSessions)
	{
		if (session.SessionInfo.IsValid() && session.SessionInfo->IsValid())
			galaxy::api::Matchmaking()->LeaveLobby(AsUniqueNetIDGOG(session.SessionInfo->GetSessionId()));
	}
}

bool FOnlineSessionGOG::CreateSession(int32 InHostingPlayerNum, FName InSessionName, const FOnlineSessionSettings& InSessionSettings)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::CreateSession: sessionName='%s'"), *InSessionName.ToString());

	CheckLocalUserNum(InHostingPlayerNum);

	if (InSessionSettings.NumPublicConnections < 0)
	{
		UE_LOG_ONLINE(Error, TEXT("Invalid number of public connections"));
		return false;
	}

	if (InSessionSettings.NumPrivateConnections > 0)
	{
		UE_LOG_ONLINE(Warning, TEXT("Private connections are not supported for GOG sessions"));
	}

	if (InSessionSettings.bIsLANMatch)
		UE_LOG_ONLINE(Warning, TEXT("LAN matches are not supported on GOG platform"));

	if (InSessionSettings.bIsDedicated)
		UE_LOG_ONLINE(Warning, TEXT("Dedicated servers are not implemented yet by GOG platform"));

	if (GetNamedSession(InSessionName) != nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("Cannot create the same session twice: sessionName='%s'"), *InSessionName.ToString());
		TriggerOnCreateSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	auto listener = CreateListener<FCreateLobbyListener>(InSessionName, InSessionSettings);

	galaxy::api::Matchmaking()->CreateLobby(
		GetLobbyType(InSessionSettings),
		InSessionSettings.NumPublicConnections,
		true,
		galaxy::api::LOBBY_TOPOLOGY_TYPE_STAR);

	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to create lobby[0]: %s; %s"), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));

		FreeListener(listener);

		TriggerOnCreateSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	return true;
}

bool FOnlineSessionGOG::CreateSession(const FUniqueNetId& InHostingPlayerId, FName InSessionName, const FOnlineSessionSettings& InSessionSettings)
{
	return CreateSession(LOCAL_USER_NUM, std::move(InSessionName), InSessionSettings);
}

galaxy::api::LobbyType FOnlineSessionGOG::GetLobbyType(const FOnlineSessionSettings& InSessionSettings)
{
	if (InSessionSettings.bShouldAdvertise)
		return galaxy::api::LOBBY_TYPE_PUBLIC;

	if (InSessionSettings.bAllowJoinViaPresenceFriendsOnly)
		return galaxy::api::LOBBY_TYPE_FRIENDS_ONLY;

	return galaxy::api::LOBBY_TYPE_PRIVATE;
}

FNamedOnlineSession* FOnlineSessionGOG::AddNamedSession(FName InSessionName, const FOnlineSessionSettings& InSessionSettings)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::AddNamedSession('%s')"), *InSessionName.ToString());

	return CreateNamedSession(std::move(InSessionName), InSessionSettings);
}

FNamedOnlineSession* FOnlineSessionGOG::AddNamedSession(FName InSessionName, const FOnlineSession& InSession)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::AddNamedSession('%s')"), *InSessionName.ToString());

	return CreateNamedSession(std::move(InSessionName), InSession);
}

FNamedOnlineSession* FOnlineSessionGOG::GetNamedSession(FName InSessionName)
{
	return const_cast<FNamedOnlineSession*>(static_cast<const FOnlineSessionGOG*>(this)->GetNamedSession(std::move(InSessionName)));
}

const FNamedOnlineSession* FOnlineSessionGOG::GetNamedSession(FName InSessionName) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::GetNamedSession('%s')"), *InSessionName.ToString());

	return storedSessions.FindByPredicate([&](const auto& session) {
		return session.SessionName == InSessionName;
	});
}

void FOnlineSessionGOG::RemoveNamedSession(FName InSessionName)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::RemoveNamedSession('%s')"), *InSessionName.ToString());

	storedSessions.RemoveAllSwap([&](const auto& session) {
		return session.SessionName == InSessionName;
	});
}

bool FOnlineSessionGOG::HasPresenceSession()
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::HasPresenceSession()"));

	return storedSessions.FindByPredicate([&](auto& session) {
		return session.SessionSettings.bUsesPresence;
	}) != nullptr;
}

EOnlineSessionState::Type FOnlineSessionGOG::GetSessionState(FName InSessionName) const
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::GetSessionState('%s')"), *InSessionName.ToString());

	auto storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
		return EOnlineSessionState::NoSession;

	return storedSession->SessionState;
}

bool FOnlineSessionGOG::StartSession(FName InSessionName)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::StartSession('%s')"), *InSessionName.ToString());

	FNamedOnlineSession* storedSession = GetNamedSession(InSessionName);
	if (storedSession == nullptr)
	{
		UE_LOG_ONLINE(Error, TEXT("Cannot start a session as it does not exist: sessionName='%s'"), *InSessionName.ToString());

		TriggerOnStartSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	if (storedSession->SessionState == EOnlineSessionState::Starting)
	{
		UE_LOG_ONLINE(Warning, TEXT("Session is already marked as starting. Skipping: sessionName='%s'"), *InSessionName.ToString());
		// Delegate will be triggered by the initial call
		return false;
	}

	if (storedSession->SessionState != EOnlineSessionState::Pending
		&& storedSession->SessionState != EOnlineSessionState::Ended)
	{
		UE_LOG_ONLINE(Warning, TEXT("Cannot start a session in current state: sessionName='%s', state='%s'"), *InSessionName.ToString(), EOnlineSessionState::ToString(storedSession->SessionState));

		TriggerOnStartSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	if (!storedSession->SessionInfo.IsValid() || !storedSession->SessionInfo->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Ivalid session info"));
		TriggerOnStartSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	storedSession->SessionState = EOnlineSessionState::Starting;

	galaxy::api::GalaxyID lobbyID = AsUniqueNetIDGOG(storedSession->SessionInfo->GetSessionId());

	auto lobbyJoinable = galaxy::api::Matchmaking()->IsLobbyJoinable(lobbyID);
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to get lobby joinability: sessionName='%s', lobbyID=%llus, %s; %s"),
			*InSessionName.ToString(), lobbyID.ToUint64(), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));

		storedSession->SessionState = EOnlineSessionState::Pending;
		TriggerOnStartSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	if (lobbyJoinable == storedSession->SessionSettings.bAllowJoinInProgress)
	{
		storedSession->SessionState = EOnlineSessionState::InProgress;
		TriggerOnStartSessionCompleteDelegates(InSessionName, true);
		return true;
	}

	auto listener = CreateListener<FLobbyStartListener>(lobbyID, InSessionName);

	galaxy::api::Matchmaking()->SetLobbyJoinable(lobbyID, storedSession->SessionSettings.bAllowJoinInProgress);
	err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to change lobby joinability: lobbyID=%llus, %s; %s"), lobbyID.ToUint64(), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));

		FreeListener(listener);
		storedSession->SessionState = EOnlineSessionState::Pending;
		TriggerOnStartSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	return true;
}

bool FOnlineSessionGOG::UpdateSession(FName InSessionName, FOnlineSessionSettings& InUpdatedSessionSettings, bool InShouldRefreshOnlineData)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::UpdateSession('%s')"), *InSessionName.ToString());

	UE_LOG_ONLINE(Error, TEXT("UpdateSession is not implemented yet"));

	TriggerOnUpdateSessionCompleteDelegates(InSessionName, false);
	return false;
}

bool FOnlineSessionGOG::EndSession(FName InSessionName)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::EndSession('%s')"), *InSessionName.ToString());

	auto* storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Warning, TEXT("Cannot end a session that does not exists: sessionName='%s'"), *InSessionName.ToString());
		TriggerOnEndSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	if (storedSession->SessionState != EOnlineSessionState::InProgress)
	{
		UE_LOG_ONLINE(Warning, TEXT("Cannot end a session that was not started: sessionName='%s', state='%s'"), *InSessionName.ToString(), EOnlineSessionState::ToString(storedSession->SessionState));
		TriggerOnEndSessionCompleteDelegates(InSessionName, false);
		return false;
	}

	storedSession->SessionState = EOnlineSessionState::Ending;

	// TODO: flush leaderboards. Then mark session as ended and trigger delegate

	storedSession->SessionState = EOnlineSessionState::Ended;

	TriggerOnEndSessionCompleteDelegates(InSessionName, true);
	return true;
}

bool FOnlineSessionGOG::DestroySession(FName InSessionName, const FOnDestroySessionCompleteDelegate& InCompletionDelegate)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::EndSession('%s')"), *InSessionName.ToString());

	FNamedOnlineSession* storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Warning, TEXT("Trying to destroy a session that does not exist: sessionName='%s'"), *InSessionName.ToString());
		InCompletionDelegate.ExecuteIfBound(InSessionName, false);
		TriggerOnDestroySessionCompleteDelegates(InSessionName, false);
		return false;
	}

	if (storedSession->SessionState == EOnlineSessionState::Destroying)
	{
		UE_LOG_ONLINE(Warning, TEXT("Session is already marked to be destroyed. Skipping: sessionName='%s'"), *InSessionName.ToString());
		// Initial operation will trigger the delegates
		return false;
	}

	if (storedSession->SessionState == EOnlineSessionState::Starting
		|| storedSession->SessionState == EOnlineSessionState::InProgress)
	{
		UE_LOG_ONLINE(Warning, TEXT("Destroying a '%s' session: sessionName='%s'"), EOnlineSessionState::ToString(storedSession->SessionState), *InSessionName.ToString());
	}

	storedSession->SessionState = EOnlineSessionState::Destroying;

	// TODO: flush leaderboards. Then mark session as destroyed and trigger delegate

	if (!storedSession->SessionInfo.IsValid()
		|| !storedSession->SessionInfo->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid session info"));
		InCompletionDelegate.ExecuteIfBound(InSessionName, false);
		TriggerOnDestroySessionCompleteDelegates(InSessionName, false);
		return false;
	}

	galaxy::api::Matchmaking()->LeaveLobby(AsUniqueNetIDGOG(storedSession->SessionInfo->GetSessionId()));
	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_ONLINE(Error, TEXT("Failed to leave lobby: lobbyID=%llu, %s; %s"), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));

	RemoveNamedSession(InSessionName);

	InCompletionDelegate.ExecuteIfBound(InSessionName, true);
	TriggerOnDestroySessionCompleteDelegates(InSessionName, true);
	return true;
}

bool FOnlineSessionGOG::IsPlayerInSession(FName InSessionName, const FUniqueNetId& InUniqueId)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::IsPlayerInSession('%s', '%s')"), *InSessionName.ToString(), *InUniqueId.ToString());

	auto storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Error, TEXT("Session not found"));
		return false;
	}

	return storedSession->RegisteredPlayers.ContainsByPredicate([&](const auto& RegisterePlayer) {
		return *RegisterePlayer == InUniqueId;
	});
}

bool FOnlineSessionGOG::StartMatchmaking(const TArray<TSharedRef<const FUniqueNetId>>&, FName InSessionName, const FOnlineSessionSettings&, TSharedRef<FOnlineSessionSearch>&)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::StartMatchmaking('%s')"), *InSessionName.ToString());

	UE_LOG_ONLINE(Error, TEXT("Matchmaking is not implemented yet"));
	TriggerOnMatchmakingCompleteDelegates(InSessionName, false);

	return false;
}

bool FOnlineSessionGOG::CancelMatchmaking(int32, FName InSessionName)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::CancelMatchmaking('%s')"), *InSessionName.ToString());

	UE_LOG_ONLINE(Error, TEXT("Matchmaking is not implemented yet"));
	TriggerOnCancelMatchmakingCompleteDelegates(InSessionName, false);

	return false;
}

bool FOnlineSessionGOG::CancelMatchmaking(const FUniqueNetId&, FName InsessionName)
{
	return CancelMatchmaking(LOCAL_USER_NUM, InsessionName);
}

bool FOnlineSessionGOG::FindSessions(int32 InSearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& InOutSearchSettings)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::FindSessions()"));

	CheckLocalUserNum(InSearchingPlayerNum);

	if (InOutSearchSettings->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		UE_LOG_ONLINE(Warning, TEXT("A session search is already pending. Ignoring this request"));
		// Delegate will be triggered by initial call
		return false;
	}

	if (InOutSearchSettings->MaxSearchResults < 0)
		UE_LOG_ONLINE(Warning, TEXT("Invalid number of max search results. Ignoring this value"));

	if (InOutSearchSettings->bIsLanQuery)
		UE_LOG_ONLINE(Warning, TEXT("LAN is not supported by GOG platform. Ignoring this value"));

	if (InOutSearchSettings->TimeoutInSeconds > 0)
		UE_LOG_ONLINE(Warning, TEXT("Search timeout is not supported. Ignoring this value"));

	if (InOutSearchSettings->PingBucketSize > 0)
		UE_LOG_ONLINE(Warning, TEXT("Ping-based search is not supported. Ignoring this value"));

	// TODO: parse search settings and employ them as session filters when session settings
	// TODO: handle default filters (SEARCH_DEDICATED_ONLY, SEARCH_EMPTY_SERVERS_ONLY, SEARCH_SECURE_SERVERS_ONLY e.t.c.) from OnlineSessionSettings.h

	// TODO: Simultaneous search operation will fill improper search result. Fix by using specific listeners
	auto listener = CreateListener<FRequestLobbyListListener>(InOutSearchSettings);

	galaxy::api::Matchmaking()->RequestLobbyList();
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to request lobby list: %s; %s"), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));

		FreeListener(listener);

		TriggerOnFindSessionsCompleteDelegates(false);
		false;
	}

	InOutSearchSettings->SearchState = EOnlineAsyncTaskState::InProgress;

	return true;
}

bool FOnlineSessionGOG::FindSessions(const FUniqueNetId& InSearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& InOutSearchSettings)
{
	return FindSessions(LOCAL_USER_NUM, InOutSearchSettings);
}

bool FOnlineSessionGOG::FindSessionById(const FUniqueNetId&, const FUniqueNetId& InSessionId, const FUniqueNetId&, const FOnSingleSessionResultCompleteDelegate& InCompletionDelegates)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::FindSessionById: sessionID='%s'"), *InSessionId.ToString());

	UE_LOG_ONLINE(Error, TEXT("FindSessionById is not implemented yet"));

	InCompletionDelegates.ExecuteIfBound(LOCAL_USER_NUM, false, {});
	TriggerOnFindSessionsCompleteDelegates(false);

	return false;
}

bool FOnlineSessionGOG::CancelFindSessions()
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::CancelFindSessions()"));

	UE_LOG_ONLINE(Error, TEXT("CancelFindSessions is not implemented yet"));

	TriggerOnCancelFindSessionsCompleteDelegates(false);
	return false;
}

bool FOnlineSessionGOG::PingSearchResults(const FOnlineSessionSearchResult& InSearchResult)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::PingSearchResults('%s')"), *InSearchResult.GetSessionIdStr());

	UE_LOG_ONLINE(Error, TEXT("PingSearchResults is not avaialable yet on GOG platform"));

	TriggerOnPingSearchResultsCompleteDelegates(false);
	return false;
}

bool FOnlineSessionGOG::JoinSession(int32 InPlayerNum, FName InSessionName, const FOnlineSessionSearchResult& InDesiredSession)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::JoinSession('%s', '%s')"), *InSessionName.ToString(), *InDesiredSession.GetSessionIdStr());

	if (GetNamedSession(InSessionName) != nullptr)
	{
		UE_LOG_ONLINE(Warning, TEXT("Cannot join a session twice: joiningSession='%s'"), *InSessionName.ToString());
		TriggerOnJoinSessionCompleteDelegates(InSessionName, EOnJoinSessionCompleteResult::AlreadyInSession);
		return false;
	}

	if (!InDesiredSession.IsValid())
	{
		UE_LOG_ONLINE(Error, TEXT("Invalid session"));
		TriggerOnJoinSessionCompleteDelegates(InSessionName, EOnJoinSessionCompleteResult::CouldNotRetrieveAddress);
		return false;
	}

	const auto& sessionID = InDesiredSession.Session.SessionInfo->GetSessionId();

	// TODO: Simultaneous create operation might call improper delegates. Fix by using specific listeners
	auto listener = CreateListener<FJoinLobbyListener>(AsUniqueNetIDGOG(sessionID), InSessionName, InDesiredSession.Session);

	galaxy::api::Matchmaking()->JoinLobby(AsUniqueNetIDGOG(sessionID));
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Warning, TEXT("Failed to join lobby: lobbyID=%s; %s; %s"), *sessionID.ToString(), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));
		TriggerOnJoinSessionCompleteDelegates(InSessionName, EOnJoinSessionCompleteResult::UnknownError);

		FreeListener(listener);

		return false;
	}

	return true;
}

bool FOnlineSessionGOG::JoinSession(const FUniqueNetId& InPlayerId, FName InSessionName, const FOnlineSessionSearchResult& InDesiredSession)
{
	return JoinSession(LOCAL_USER_NUM, InSessionName, InDesiredSession);
}

bool FOnlineSessionGOG::FindFriendSession(int32 InLocalUserNum, const FUniqueNetId&)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::FindFriendSession()"));

	UE_LOG_ONLINE(Error, TEXT("FindFriendSession is not supported by GOG platform"));

	TriggerOnFindFriendSessionCompleteDelegates(InLocalUserNum, false, {});

	return false;
}

bool FOnlineSessionGOG::FindFriendSession(const FUniqueNetId&, const FUniqueNetId&)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::FindFriendSession()"));

	UE_LOG_ONLINE(Error, TEXT("FindFriendSession is not supported by GOG platform"));

	TriggerOnFindFriendSessionCompleteDelegates(LOCAL_USER_NUM, false, {});

	return false;
}

bool FOnlineSessionGOG::FindFriendSession(const FUniqueNetId&, const TArray<TSharedRef<const FUniqueNetId>>&)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::FindFriendSession()"));

	UE_LOG_ONLINE(Error, TEXT("FindFriendSession is not supported by GOG platform"));

	TriggerOnFindFriendSessionCompleteDelegates(LOCAL_USER_NUM, false, {});

	return false;
}

bool FOnlineSessionGOG::SendSessionInviteToFriend(int32 InLocalUserNum, FName InSessionName, const FUniqueNetId& InFriend)
{
	return SendSessionInviteToFriends(InLocalUserNum, InSessionName, {MakeShareable<FUniqueNetIdGOG>(new FUniqueNetIdGOG{InFriend})});
}

bool FOnlineSessionGOG::SendSessionInviteToFriend(const FUniqueNetId&, FName InSessionName, const FUniqueNetId& InFriend)
{
	return SendSessionInviteToFriends(LOCAL_USER_NUM, InSessionName, {MakeShareable<FUniqueNetIdGOG>(new FUniqueNetIdGOG{InFriend})});
}

bool FOnlineSessionGOG::SendSessionInviteToFriends(int32 InLocalUserNum, FName InSessionName, const TArray<TSharedRef<const FUniqueNetId>>& InFriends)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::SendSessionInviteToFriends('%s', %d)"), *InSessionName.ToString(), InFriends.Num());

	auto storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid session"));
		return false;
	}

	if (!storedSession->SessionInfo.IsValid() || !storedSession->SessionInfo->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid session info"));
		return false;
	}

	FString connectString;
	if (!GetResolvedConnectString(InSessionName, connectString))
		return false;

	connectString.Append(TEXT("?SessionName=")).Append(InSessionName.ToString());

	bool invitationSentSuccessfully = true;
	for (auto invitedFriend : InFriends)
	{
		galaxy::api::Friends()->SendInvitation(FUniqueNetIdGOG(*invitedFriend), TCHAR_TO_ANSI(*connectString));
		auto err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_ONLINE(Warning, TEXT("Failed to invite a friend: friendId='%s'; %s; %s"), *invitedFriend->ToDebugString(), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));
			invitationSentSuccessfully = false;
		}
	}

	return invitationSentSuccessfully;
}

bool FOnlineSessionGOG::SendSessionInviteToFriends(const FUniqueNetId&, FName InSessionName, const TArray<TSharedRef<const FUniqueNetId>>& InFriends)
{
	return SendSessionInviteToFriends(LOCAL_USER_NUM, InSessionName, InFriends);
}

bool FOnlineSessionGOG::GetResolvedConnectStringFromSession(const FOnlineSession& InSession, FString& OutConnectString) const
{
	if (!InSession.SessionInfo.IsValid()
		|| !InSession.SessionInfo->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid SessionInfo"));
		return false;
	}

	OutConnectString = FInternetAddrGOG{InSession.SessionInfo->GetSessionId()}.ToString(true);
	return true;
}

bool FOnlineSessionGOG::GetResolvedConnectString(FName InSessionName, FString& OutConnectInfo, FName)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::GetResolvedConnectString('%s')"), *InSessionName.ToString());

	auto storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Warning, TEXT("Session info not found"));
		return false;
	}

	return GetResolvedConnectStringFromSession(*storedSession, OutConnectInfo);
}

bool FOnlineSessionGOG::GetResolvedConnectString(const FOnlineSessionSearchResult& InSearchResult, const FName, FString& OutConnectInfo)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::GetResolvedConnectString()"));

	if (!InSearchResult.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid session info"));
		return false;
	}

	return GetResolvedConnectStringFromSession(InSearchResult.Session, OutConnectInfo);
}

FOnlineSessionSettings* FOnlineSessionGOG::GetSessionSettings(FName InSessionName)
{
	UE_LOG_ONLINE(VeryVerbose, TEXT("FOnlineSessionGOG::GetResolvedConnectString('%s')"), *InSessionName.ToString());

	auto storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Warning, TEXT("Session not found"));
		return nullptr;
	}

	return &storedSession->SessionSettings;
}

bool FOnlineSessionGOG::RegisterPlayer(FName InSessionName, const FUniqueNetId& InPlayerID, bool InWasInvited)
{
	return RegisterPlayers(InSessionName, {MakeShared<FUniqueNetIdGOG>(InPlayerID)}, InWasInvited);
}

bool FOnlineSessionGOG::RegisterPlayers(FName InSessionName, const TArray< TSharedRef<const FUniqueNetId> >& InPlayers, bool InWasInvited)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::RegisterPlayers('%s', %d)"), *InSessionName.ToString(), InPlayers.Num());

	auto* storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Error, TEXT("Session not found"));

		TriggerOnRegisterPlayersCompleteDelegates(InSessionName, InPlayers, false);
		return false;
	}

	if (!storedSession->SessionInfo.IsValid() || !storedSession->SessionInfo->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Invalid session info"));

		TriggerOnRegisterPlayersCompleteDelegates(InSessionName, InPlayers, false);
		return false;
	}

	for (const auto& playerID : InPlayers)
	{
		if (storedSession->RegisteredPlayers.Find(playerID) != INDEX_NONE)
		{
			UE_LOG_ONLINE(Verbose, TEXT("A player already registered in a session. Skipping: playerID=%s, sessionName='%s'"), *playerID->ToDebugString(), *InSessionName.ToString());
			continue;
		}

		if (!playerID->IsValid())
			UE_LOG_ONLINE(Warning, TEXT("Invalid player ID"));

		storedSession->RegisteredPlayers.Add(playerID);

		galaxy::api::Friends()->RequestUserInformation(AsUniqueNetIDGOG(*playerID));
		auto err = galaxy::api::GetError();
		if (err)
			UE_LOG_ONLINE(Warning, TEXT("Failed to request information for user: userID='%s'; '%s'; '%s'"), *playerID->ToString(), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));
	}

	TriggerOnRegisterPlayersCompleteDelegates(InSessionName, InPlayers, true);
	return true;
}

bool FOnlineSessionGOG::UnregisterPlayer(FName InSessionName, const FUniqueNetId& InPlayerId)
{
	return UnregisterPlayers(InSessionName, {MakeShared<FUniqueNetIdGOG>(InPlayerId)});
}

bool FOnlineSessionGOG::UnregisterPlayers(FName InSessionName, const TArray<TSharedRef<const FUniqueNetId>>& InPlayers)
{
	UE_LOG_ONLINE(Warning, TEXT("FOnlineSessionGOG::UnregisterPlayers('%s', %d)"), *InSessionName.ToString(), InPlayers.Num());

	auto* storedSession = GetNamedSession(InSessionName);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Error, TEXT("Session not found"));

		TriggerOnUnregisterPlayersCompleteDelegates(InSessionName, InPlayers, false);
		return false;
	}

	if (!storedSession->SessionInfo.IsValid() || !storedSession->SessionInfo->IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("Session info is invalid: sessionName='%s'"), *InSessionName.ToString());

		TriggerOnUnregisterPlayersCompleteDelegates(InSessionName, InPlayers, false);
		return false;
	}

	for (const auto& playerID : InPlayers)
	{
		auto playerIdx = storedSession->RegisteredPlayers.Find(playerID);
		if (playerIdx == INDEX_NONE)
		{
			UE_LOG_ONLINE(Verbose, TEXT("Player is not registered for a session. Skipping: playerID=%s, sessionName='%s'"), *playerID->ToDebugString(), *InSessionName.ToString());
			continue;
		}

		storedSession->RegisteredPlayers.RemoveAtSwap(playerIdx);
	}

	TriggerOnUnregisterPlayersCompleteDelegates(InSessionName, InPlayers, true);
	return true;
}

void FOnlineSessionGOG::RegisterLocalPlayer(const FUniqueNetId& InPlayerId, FName InSessionName, const FOnRegisterLocalPlayerCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::RegisterLocalPlayer('%s', '%s')"), *InSessionName.ToString(), *InPlayerId.ToString());

	InDelegate.ExecuteIfBound(InPlayerId, EOnJoinSessionCompleteResult::Success);
}

void FOnlineSessionGOG::UnregisterLocalPlayer(const FUniqueNetId& InPlayerId, FName InSessionName, const FOnUnregisterLocalPlayerCompleteDelegate& InDelegate)
{
	UE_LOG_ONLINE(Display, TEXT("FOnlineSessionGOG::UnregisterLocalPlayer('%s', '%s')"), *InSessionName.ToString(), *InPlayerId.ToString());

	InDelegate.ExecuteIfBound(InPlayerId, true);
}

int32 FOnlineSessionGOG::GetNumSessions()
{
	UE_LOG_ONLINE(VeryVerbose, TEXT("GetNumSessions"));

	return storedSessions.Num();
}

void FOnlineSessionGOG::OnLobbyLeft(const galaxy::api::GalaxyID& InLobbyID, bool InIoFailure)
{
	UE_LOG_ONLINE(Log, TEXT("FOnlineSessionGOG::OnLobbyLeft()"));

	auto storedSession = FindSession(InLobbyID);
	if (!storedSession)
	{
		UE_LOG_ONLINE(Warning, TEXT("Lobby left listener called for an unknown session. Ignoring"));
		return;
	}

	subsystemGOG.TriggerOnConnectionStatusChangedDelegates(EOnlineServerConnectionStatus::Normal, EOnlineServerConnectionStatus::ConnectionDropped);

	// Not sure if we have to clean this up, or developer/Engine will manage everything, but let's do it
	DestroySession(storedSession->SessionName);
}

void FOnlineSessionGOG::DumpSessionState()
{
	UE_LOG_ONLINE(Display, TEXT("DumpSessionState"));

	for (const auto& session : storedSessions)
		DumpNamedSession(&session);
}

const FNamedOnlineSession* FOnlineSessionGOG::FindSession(const FUniqueNetIdGOG& InSessionID) const
{
	return storedSessions.FindByPredicate([&](const auto& session) {
		const auto& sessionInfo = session.SessionInfo;
		return sessionInfo.IsValid() && sessionInfo->IsValid() && sessionInfo->GetSessionId() == InSessionID;
	});
}
