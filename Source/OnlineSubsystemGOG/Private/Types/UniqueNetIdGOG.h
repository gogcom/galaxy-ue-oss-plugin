#pragma once

#include "CommonGOG.h"

class FUniqueNetIdGOG : public FUniqueNetId
{
public:

	explicit FUniqueNetIdGOG(uint64 InID);

	explicit FUniqueNetIdGOG(const FString& InStr);

	explicit FUniqueNetIdGOG(const FUniqueNetIdGOG& InUniqueNetIdGOG);

	FUniqueNetIdGOG(const uint8* InBytes, int32 InSize);

	FUniqueNetIdGOG(const galaxy::api::GalaxyID& InGalaxyID);

	FUniqueNetIdGOG(const FUniqueNetId& InUniqueNetId);

	virtual const uint8* GetBytes() const override;

	virtual int32 GetSize() const override;

	virtual bool IsValid() const override;

	virtual FString ToString() const override;

	virtual FString ToDebugString() const override;

	friend uint32 GetTypeHash(const FUniqueNetIdGOG& InUniqueNetIdGOG);

	operator galaxy::api::GalaxyID() const;

	FUniqueNetIdGOG& operator=(const galaxy::api::GalaxyID& InGalaxyID);

PACKAGE_SCOPE:

	FUniqueNetIdGOG() = default;

private:

	uint64 id;
};

// Used to avoid temporary object creation
inline const FUniqueNetIdGOG& AsUniqueNetIDGOG(const FUniqueNetId& InUniqueNetID)
{
	return dynamic_cast<const FUniqueNetIdGOG&>(InUniqueNetID);
}