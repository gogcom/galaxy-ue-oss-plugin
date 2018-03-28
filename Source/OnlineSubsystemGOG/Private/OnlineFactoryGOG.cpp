#include "OnlineFactoryGOG.h"

#include "Misc/ConfigCacheIni.h"

FOnlineFactoryGOG::FOnlineFactoryGOG()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineFactoryGOG::ctor()"));
}

IOnlineSubsystemPtr FOnlineFactoryGOG::CreateSubsystem(FName InInstanceName)
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineFactoryGOG::CreateSubsystem(%s)"), *InInstanceName.ToString());

	return nullptr;
}

FOnlineFactoryGOG::~FOnlineFactoryGOG()
{
	UE_LOG_ONLINE(Verbose, TEXT("FOnlineFactoryGOG::dtor()"));
}