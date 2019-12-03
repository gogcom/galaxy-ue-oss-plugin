#include "UniqueNetIdGOG.h"
#include "Loggers.h"

namespace
{
FString sanitizeID(const FString& InStr)
{
	if (InStr.StartsWith(TEXT("GOG.")))
		return InStr.Mid(ARRAY_COUNT(TEXT("GOG.")) - 1);

	return InStr;
}
}

FUniqueNetIdGOG::FUniqueNetIdGOG(uint64 InId)
	: id{InId}
{
}

FUniqueNetIdGOG::FUniqueNetIdGOG(const galaxy::api::GalaxyID& InGalaxyID)
	: id{InGalaxyID.ToUint64()}
{
}

FUniqueNetIdGOG::FUniqueNetIdGOG(const FString& InStr)
	: id{static_cast<uint64>(FCString::Atoi64(*sanitizeID(InStr)))}
{
}

FUniqueNetIdGOG::FUniqueNetIdGOG(const FUniqueNetIdGOG& InUniqueNetIdGOG)
	: id{InUniqueNetIdGOG.id}
{
}

FUniqueNetIdGOG::FUniqueNetIdGOG(const FUniqueNetId& InUniqueNetId)
	: FUniqueNetIdGOG{InUniqueNetId.GetBytes(), InUniqueNetId.GetSize()}
{
}

FUniqueNetIdGOG::FUniqueNetIdGOG(const uint8* InBytes, int32 InSize)
	: id{*reinterpret_cast<const uint64*>(InBytes)}
{
	check(InSize == GetSize());
}

const uint8* FUniqueNetIdGOG::GetBytes() const
{
	return reinterpret_cast<const uint8*>(&id);
}

int32 FUniqueNetIdGOG::GetSize() const
{
	return sizeof(id);
}

bool FUniqueNetIdGOG::IsValid() const
{
	return galaxy::api::GalaxyID{id}.IsValid();
}

bool FUniqueNetIdGOG::IsUser() const
{
	return galaxy::api::GalaxyID{id}.GetIDType() == galaxy::api::GalaxyID::ID_TYPE_USER;
}

bool FUniqueNetIdGOG::IsLobby() const
{
	return galaxy::api::GalaxyID{id}.GetIDType() == galaxy::api::GalaxyID::ID_TYPE_LOBBY;
}

FString FUniqueNetIdGOG::ToString() const
{
	return FString::Printf(TEXT("%llu"), id);
}

FString FUniqueNetIdGOG::ToDebugString() const
{
	galaxy::api::GalaxyID tmpGalaxyID{id};
	switch (tmpGalaxyID.GetIDType())
	{
		case galaxy::api::GalaxyID::ID_TYPE_USER:
			return FString::Printf(TEXT("User GalaxyID: %llu"), id);

		case galaxy::api::GalaxyID::ID_TYPE_LOBBY:
			return FString::Printf(TEXT("Lobby GalaxyID: %llu"), id);

		case galaxy::api::GalaxyID::ID_TYPE_UNASSIGNED:
			return FString::Printf(TEXT("Unassigned GalaxyID: %llu"), id);

		default:
			UE_LOG_ONLINE(Error, TEXT("Invalid GalaxyID type: type=%u, id=%llu"), static_cast<uint32>(tmpGalaxyID.GetIDType()), id);
			return FString::Printf(TEXT("Invalid GalaxyID type: type=%u, id=%llu"), static_cast<uint32>(tmpGalaxyID.GetIDType()), id);;
	}
}

uint32 GetTypeHash(const FUniqueNetIdGOG& InUniqueNetId)
{
	return GetTypeHash(InUniqueNetId.id);
}

FUniqueNetIdGOG::operator galaxy::api::GalaxyID() const
{
	return galaxy::api::GalaxyID{id};
}

FUniqueNetIdGOG& FUniqueNetIdGOG::operator=(const galaxy::api::GalaxyID& InGalaxyID)
{
	id = InGalaxyID.ToUint64();

	return *this;
}

bool FUniqueNetIdGOG::operator==(const galaxy::api::GalaxyID& InGalaxyID) const
{
	return id == InGalaxyID.ToUint64();
}

FArchive& operator<<(FArchive& InArchive, FUniqueNetIdGOG& InUniqueNetId)
{
	return InArchive << InUniqueNetId.id;
}

#if ENGINE_MINOR_VERSION >= 20
FName FUniqueNetIdGOG::GetType() const
{
	return TEXT_GOG;
}
#endif
