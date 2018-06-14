#include "OnlineSubsystemGOG.h"

#include "Identity/OnlineIdentityGOG.h"

#include "SharedPointer.h"

FOnlineSubsystemGOG::FOnlineSubsystemGOG()
{
}

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
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::ReadStrFromConfig(%s)"), ANSI_TO_TCHAR(InKey));

	FString str = GConfig->GetStr(TEXT_ONLINE_SUBSYSTEM_GOG, ANSI_TO_TCHAR(InKey), GEngineIni);
	if (str.IsEmpty())
	{
		UE_LOG_ONLINE(Error, TEXT("%s missing parameter: %s"), *GEngineIni, ANSI_TO_TCHAR(InKey));
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

FText FOnlineSubsystemGOG::GetOnlineServiceName() const
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::GetOnlineServiceName()"));

	return NSLOCTEXT("OnlineSubsystemGOG", "OnlineServiceName", "GOG");
}

bool FOnlineSubsystemGOG::Tick(float InDeltaTime)
{
	UE_LOG_ONLINE(VeryVerbose, TEXT("OnlineSubsystemGOG::Tick()"));

	galaxy::api::ProcessData();
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Error, TEXT("Failed to tick galaxy::api::ProcessData()"));
		return false;
	}

	return FOnlineSubsystemImpl::Tick(InDeltaTime);
}

bool FOnlineSubsystemGOG::InitGalaxyPeer()
{
	UE_LOG_ONLINE(Display, TEXT("OnlineSubsystemGOG::InitGalaxyPeer()"));

	galaxy::api::Init(galaxy::api::InitOptions{TCHAR_TO_ANSI(*GetAppId()), TCHAR_TO_ANSI(*GetClientSecret())});
	auto err = galaxy::api::GetError();
	if (err)
	{
		UE_LOG_ONLINE(Error, TEXT("Galaxy SDK failed to initialize: %s; %s"), ANSI_TO_TCHAR(err->GetName()), ANSI_TO_TCHAR(err->GetMsg()));
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

#define DEFINE_ONLINE_INTERFACE(interfaceName) \
	galaxy##interfaceName##Interface = MakeShared<FOnline##interfaceName##GOG, ESPMode::ThreadSafe>(*this); \
	\
	check(galaxy##interfaceName##Interface.IsValid())

	DEFINE_ONLINE_INTERFACE(Identity);

	// TODO: create interfaces here

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

bool FOnlineSubsystemGOG::FOnlineSubsystemGOG::IsInitialized()
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

	// TODO: release interfaces here

	galaxyIdentityInterface.Reset();

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
