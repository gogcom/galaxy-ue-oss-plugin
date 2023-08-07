#pragma once

#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNetworkingGOG, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogTrafficGOG, Log, All);

#define UE_LOG_NETWORKING(Verbosity, Format, ...) UE_LOG(LogNetworkingGOG, Verbosity, Format,  ##__VA_ARGS__)
#define UE_LOG_TRAFFIC(Verbosity, Format, ...) UE_LOG(LogTrafficGOG, Verbosity, Format,  ##__VA_ARGS__)

#if ENGINE_MINOR_VERSION < 20 && ENGINE_MAJOR_VERSION < 5
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineAchievements, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineFriend, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineIdentity, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineLeaderboard, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogOnlinePresence, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineSession, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogOnlineExternalUI, Log, All);

#define UE_LOG_ONLINE_ACHIEVEMENTS(Verbosity, Format, ...) { \
	UE_LOG(LogOnlineAchievements, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); }

#define UE_LOG_ONLINE_FRIEND(Verbosity, Format, ...) { \
	UE_LOG(LogOnlineFriend, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); }

#define UE_LOG_ONLINE_IDENTITY(Verbosity, Format, ...) { \
	UE_LOG(LogOnlineIdentity, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); }

#define UE_LOG_ONLINE_LEADERBOARD(Verbosity, Format, ...) { \
	UE_LOG(LogOnlineLeaderboard, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); }

#define UE_LOG_ONLINE_PRESENCE(Verbosity, Format, ...) { \
	UE_LOG(LogOnlinePresence, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); }

#define UE_LOG_ONLINE_SESSION(Verbosity, Format, ...) { \
	UE_LOG(LogOnlineSession, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); }

#define UE_LOG_ONLINE_EXTERNALUI(Verbosity, Format, ...) { \
	UE_LOG(LogOnlineExternalUI, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); }
#endif
