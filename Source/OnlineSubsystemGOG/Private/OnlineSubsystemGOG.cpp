#include "OnlineSubsystemGOG.h"

#include "Identity/OnlineIdentityGOG.h"
#include "Session/OnlineSessionGOG.h"
#include "Achievements/AchievementsInterfaceGOG.h"
#include "Leaderboards/OnlineLeaderboardsGOG.h"
#include "Friends/OnlineFriendsGOG.h"
#include "Presence/OnlinePresenceGOG.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Templates/SharedPointer.h"
#include <algorithm>

namespace
{

	EOnlineServerConnectionStatus::Type GetConnectionState(galaxy::api::GogServicesConnectionState InConnectionState)
	{
		using namespace galaxy::api;

		switch (InConnectionState)
		{
			case GOG_SERVICES_CONNECTION_STATE_CONNECTED:
				return EOnlineServerConnectionStatus::Connected;

			case GOG_SERVICES_CONNECTION_STATE_DISCONNECTED:
				return EOnlineServerConnectionStatus::ConnectionDropped;

			case GOG_SERVICES_CONNECTION_STATE_AUTH_LOST:
				return EOnlineServerConnectionStatus::NotAuthorized;

			default:
				checkf(false, TEXT("Unsupported connection state: %u"), InConnectionState);
			case GOG_SERVICES_CONNECTION_STATE_UNDEFINED:
				return EOnlineServerConnectionStatus::Normal;
		}
	}

	std::uint16_t GetPeerPort()
	{
		{
			std::uint16_t port{0};
			if (FParse::Value(FCommandLine::Get(), TEXT("port="), port))
				return port;
		}

		int32 port{0};
		if (GConfig->GetInt(TEXT_CONFIG_SECTION_GOG, TEXT("Port"), port, GEngineIni))
			return static_cast<std::uint16_t>(std::max(port, 0));

		return 0; // use any port
	}

	FString GetPeerHost()
	{
		FString host;
		if (FParse::Value(FCommandLine::Get(), TEXT("multihome="), host) && !host.IsEmpty())
			return host;

		host = GConfig->GetStr(TEXT_CONFIG_SECTION_GOG, TEXT("Host"), GEngineIni);
		if (!host.IsEmpty())
			return host;

		return {};
	}

}

class FOnlineSubsystemGOG::GlobalConnectionListener : public galaxy::api::GlobalGogServicesConnectionStateListener
{
public:

	GlobalConnectionListener(FOnlineSubsystemGOG& InSubsystemGOG)
		: subsystemGOG{InSubsystemGOG}
	{
	}

	void OnConnectionStateChange(galaxy::api::GogServicesConnectionState InConnectionState)
	{
		auto newState = GetConnectionState(InConnectionState);

#if ENGINE_MINOR_VERSION < 20
		subsystemGOG.TriggerOnConnectionStatusChangedDelegates(currentState, newState);
#else
		subsystemGOG.TriggerOnConnectionStatusChangedDelegates(
			subsystemGOG.GetSubsystemName().ToString(),
			currentState,
			newState
		);
#endif
		currentState = newState;
	}

private:

	EOnlineServerConnectionStatus::Type currentState{EOnlineServerConnectionStatus::Normal};
	FOnlineSubsystemGOG& subsystemGOG;
};

FOnlineSubsystemGOG::FOnlineSubsystemGOG(FName InInstanceName)
	: FOnlineSubsystemImpl(TEXT_GOG, InInstanceName)
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::ctor(%s)"), *InInstanceName.ToString());
}

FString FOnlineSubsystemGOG::GetAppId() const
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::GetAppId()"));
	check(!clientID.IsEmpty());
	return clientID;
}

FString FOnlineSubsystemGOG::GetClientSecret() const
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::GetClientSecret()"));
	check(!clientSecret.IsEmpty());
	return clientSecret;
}

FString ReadStrFromConfig(const char* InKey)
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::ReadStrFromConfig(%s)"), UTF8_TO_TCHAR(InKey));

	FString str = GConfig->GetStr(TEXT_CONFIG_SECTION_GOG, UTF8_TO_TCHAR(InKey), GEngineIni);
	if (str.IsEmpty())
	{
		UE_LOG_ONLINE(Error, TEXT("%s missing parameter: %s"), *GEngineIni, UTF8_TO_TCHAR(InKey));
		check(false && "Missing configuration parameter");
	}

	return str;
}

bool FOnlineSubsystemGOG::ReadEngineConfiguration()
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::ReadEngineConfiguration()"));

	check(clientID.IsEmpty());
	check(clientSecret.IsEmpty());

	clientID = ReadStrFromConfig("ClientID");
	clientSecret = ReadStrFromConfig("ClientSecret");

	return !clientID.IsEmpty() && !clientSecret.IsEmpty();
}

#if ENGINE_MINOR_VERSION >= 19
FText FOnlineSubsystemGOG::GetOnlineServiceName() const
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::GetOnlineServiceName()"));

	return NSLOCTEXT("OnlineSubsystemGOG", "OnlineServiceName", "GOG");
}
#endif

bool FOnlineSubsystemGOG::Tick(float InDeltaTime)
{
	UE_LOG_ONLINE(VeryVerbose, TEXT("OnlineSubsystemGOG::Tick()"));

	galaxy::api::ProcessData();
	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_ONLINE(Error, TEXT("Failed to tick ProcessData(): %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));

	return FOnlineSubsystemImpl::Tick(InDeltaTime);
}

bool FOnlineSubsystemGOG::InitGalaxyPeer()
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::InitGalaxyPeer()"));

	const auto host = GetPeerHost();
	const auto port = GetPeerPort();

	if(!host.IsEmpty() || port != 0 )
		UE_LOG_ONLINE(Display, TEXT("Binding Galaxy to adress %s:%u"), *host, port);

	galaxy::api::Init({
		TCHAR_TO_UTF8(*GetAppId()),
		TCHAR_TO_UTF8(*GetClientSecret()),
		".", // configPath
		nullptr, // galaxyAllocator
		nullptr, // storagePath
		TCHAR_TO_UTF8(*host),
		GetPeerPort()});

	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Error, TEXT("Galaxy SDK failed to initialize: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return false;
	}

	UE_LOG_ONLINE(Display, TEXT("GalaxyPeer initialized successfully: clientID=%s"), *GetAppId());
	return true;
}

bool FOnlineSubsystemGOG::Init()
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::Init()"));

	check(!IsInitialized() && "OnlineSubsystemGOG already initialized");

	if (!ReadEngineConfiguration()
		|| !InitGalaxyPeer())
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to initialize OnlineSubsystemGOG. Online features are not available."));
		return false;
	}

	globalConnectionListener = MakeUnique<GlobalConnectionListener>(*this);

	auto identityInterfaceGOG = MakeShared<FOnlineIdentityGOG, ESPMode::ThreadSafe>(*this);
	auto ownUserOnlineAccount = identityInterfaceGOG->GetOwnUserOnlineAccount();
	galaxyIdentityInterface = identityInterfaceGOG;

	galaxySessionInterface = MakeShared<FOnlineSessionGOG, ESPMode::ThreadSafe>(*this, ownUserOnlineAccount);
	galaxyAchievementsInterface = MakeShared<FOnlineAchievementsGOG, ESPMode::ThreadSafe>(*this, ownUserOnlineAccount);
	galaxyLeaderboardsInterface = MakeShared<FOnlineLeaderboardsGOG, ESPMode::ThreadSafe>(*this, ownUserOnlineAccount);
	galaxyFriendsInterface = MakeShared<FOnlineFriendsGOG, ESPMode::ThreadSafe>(*this, ownUserOnlineAccount);
	galaxyPresenceInterface = MakeShared<FOnlinePresenceGOG, ESPMode::ThreadSafe>(*this, ownUserOnlineAccount);

	// TODO: create more interfaces here

	bGalaxyPeerInitialized = true;
	return bGalaxyPeerInitialized;
}

bool FOnlineSubsystemGOG::IsLocalPlayer(const FUniqueNetId& InUniqueId) const
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::IsLocalPlayer()"));

	return FOnlineSubsystemImpl::IsLocalPlayer(InUniqueId);;
}

bool FOnlineSubsystemGOG::Exec(UWorld* InWorld, const TCHAR* InCmd, FOutputDevice& Ar)
{
	return FOnlineSubsystemImpl::Exec(InWorld, InCmd, Ar);
}

bool FOnlineSubsystemGOG::IsInitialized()
{
	return bGalaxyPeerInitialized;
}

void FOnlineSubsystemGOG::ShutdownGalaxyPeer()
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::ShutdownGalaxyPeer()"));

	if (!IsInitialized())
		UE_LOG_ONLINE(Warning, TEXT("Trying to shutdown GalaxyPeer when it was not initialized"));

	galaxy::api::Shutdown();
}

bool FOnlineSubsystemGOG::ShutdownImpl()
{
	FOnlineSubsystemImpl::Shutdown();

	// TODO: release all interfaces before shutting down

	galaxyIdentityInterface.Reset();
	galaxySessionInterface.Reset();
	galaxyAchievementsInterface.Reset();
	galaxyLeaderboardsInterface.Reset();
	galaxyFriendsInterface.Reset();
	galaxyPresenceInterface.Reset();

	globalConnectionListener.Reset();

	ShutdownGalaxyPeer();

	bGalaxyPeerInitialized = false;
	return true;
}

bool FOnlineSubsystemGOG::Shutdown()
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::Shutdown()"));
#if UE_EDITOR
	return true;
#else
	return ShutdownImpl();
#endif
}

FOnlineSubsystemGOG::~FOnlineSubsystemGOG()
{
	ShutdownImpl();
};
