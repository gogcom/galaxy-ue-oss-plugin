#pragma once

// TODO: consider replacing with "CoreMinimal.h" and "UObject/CoreOnline.h" to reduce compilation time
#include "Core.h"
#include "OnlineSubsystem.h"

#include <galaxy/GalaxyApi.h>
#include <galaxy/GalaxyExceptionHelper.h>

#define TEXT_GOG TEXT("GOG")

#define TEXT_ONLINE_SUBSYSTEM_GOG TEXT("OnlineSubsystemGOG")

#define STRINGIFY(X) STRINGIFYIMPL(X)
#define STRINGIFYIMPL(X) #X
