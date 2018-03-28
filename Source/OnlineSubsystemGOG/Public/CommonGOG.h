#pragma once

// TODO: consider replacing with "CoreMinimal.h" and "UObject/CoreOnline.h" to reduce compilation time
#include "Core.h"
#include "OnlineSubsystem.h"

#define GOG_SUBSYSTEM FName(TEXT("GOG"))

#define GOG_OSS_MODULE FName(TEXT("OnlineSubsystemGOG"))

#define STRINGIFY(X) STRINGIFYIMPL(X)
#define STRINGIFYIMPL(X) #X