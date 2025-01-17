#pragma once

#include "CommonGOG.h"
#include "Runtime/Launch/Resources/Version.h"
#include "OnlineSubsystemGOGPackage.h"

uint32 GetTypeHash(const class FUniqueNetIdGOG& InUniqueNetIdGOG);

class FUniqueNetIdGOG : public FUniqueNetId
{
public:

	explicit FUniqueNetIdGOG(uint64 InID);

	explicit FUniqueNetIdGOG(const FString& InStr);

	explicit FUniqueNetIdGOG(const FUniqueNetIdGOG& InUniqueNetIdGOG);

	FUniqueNetIdGOG(const uint8* InBytes, int32 InSize);

	FUniqueNetIdGOG(const galaxy::api::GalaxyID& InGalaxyID);

	FUniqueNetIdGOG(const FUniqueNetId& InUniqueNetId);

	const uint8* GetBytes() const override;

	int32 GetSize() const override;

	bool IsValid() const override;

	bool IsUser() const;

	bool IsLobby() const;

	FString ToString() const override;

	FString ToDebugString() const override;

	friend uint32 GetTypeHash(const FUniqueNetIdGOG& InUniqueNetIdGOG);

	operator galaxy::api::GalaxyID() const;

	FUniqueNetIdGOG& operator=(const galaxy::api::GalaxyID& InGalaxyID);

	bool operator==(const galaxy::api::GalaxyID& InGalaxyID) const;

	bool operator==(const FUniqueNetIdGOG& InGalaxyID) const;

	friend FArchive& operator<<(FArchive& InArchive, FUniqueNetIdGOG& InUniqueNetId);

#if ENGINE_MINOR_VERSION >= 20 || ENGINE_MAJOR_VERSION > 4
	FName GetType() const override;
#endif

PACKAGE_SCOPE:

	FUniqueNetIdGOG() = default;

private:

	uint64 id{galaxy::api::GalaxyID::UNASSIGNED_VALUE};
};
