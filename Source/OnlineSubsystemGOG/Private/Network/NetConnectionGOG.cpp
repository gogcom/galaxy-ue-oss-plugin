#include "NetConnectionGOG.h"

#include "CommonGOG.h"
#include "Types/UrlGOG.h"
#include "Loggers.h"

#include "Net/DataChannel.h"
#include "PacketHandlers/StatelessConnectHandlerComponent.h"

void UNetConnectionGOG::InitBase(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetConnectionGOG::InitBase()"));

	// Put temporary '1' for PacketOverhead to bypass assertion in InitBase ...
	UNetConnection::InitBase(InDriver, InSocket, InURL, InState, InMaxPacket == 0 ? MAX_PACKET_SIZE: InMaxPacket, 1);
	// ... then replace it with '0', as we handle our own overhead.
	PacketOverhead = 0;

	InitSendBuffer();
}

void UNetConnectionGOG::InitLocalConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetConnectionGOG::InitLocalConnection(URL=%s)"), *InURL.ToString(true));

	remotePeerID = FUniqueNetIdGOG{InURL.Host};
	if (!remotePeerID.IsValid() || !remotePeerID.IsLobby())
	{
		UE_LOG_NETWORKING(Error, TEXT("Remote PeerID expected to be a lobby: remotePeerID='%s', remoteUrl='%s'"),
			*remotePeerID.ToString(), *InURL.ToString(true));
		return;
	}

	InitBase(InDriver, InSocket, InURL, InState, InMaxPacket, InPacketOverhead);
}

void UNetConnectionGOG::InitRemoteConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, const class FInternetAddr& InRemoteAddr, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetConnectionGOG::InitRemoteConnection(URL=%s)"), *InURL.ToString(true));

	remotePeerID = FUniqueNetIdGOG{InURL.Host};
	if (!remotePeerID.IsValid() || !remotePeerID.IsUser())
	{
		UE_LOG_NETWORKING(Error, TEXT("Remote PeerID expected to be an user: remotePeerID='%s', remoteUrl='%s'"),
			*remotePeerID.ToString(), *InURL.ToString(true));
		return;
	}

	InitBase(InDriver, InSocket, InURL, InState, InMaxPacket, InPacketOverhead);

#if ENGINE_MINOR_VERSION >= 23
	RemoteAddr = InRemoteAddr.Clone();
#endif

	serverMemberStateListener.Reset(new LobbyMemberStateListener{*this});

	SetClientLoginState(EClientLoginState::LoggingIn);
	SetExpectedClientLoginMsgType(NMT_Hello);
}

#if ENGINE_MINOR_VERSION >= 21
#if ENGINE_MINOR_VERSION < 23
TSharedPtr<FInternetAddr> UNetConnectionGOG::GetInternetAddr()
{
	// FInternetAddr is used by the engine for fast connection mapping, mainly to prevent DDoS when using IpNetDriver.
	// We rely on the GalaxySDK, so this is not needed.
	return {};
}
#endif

void UNetConnectionGOG::LowLevelSend(void* InData, int32 InCountBits, FOutPacketTraits& OutTraits)
#else
void UNetConnectionGOG::LowLevelSend(void* InData, int32 /*InCountBytes*/, int32 InCountBits)
#endif
{
	auto dataToSend = reinterpret_cast<uint8*>(InData);

	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetConnectionGOG::LowLevelSend(): size=%u bits; data={%s}"),
		InCountBits, *BytesToHex(dataToSend, FMath::DivideAndRoundUp(InCountBits, 8)));

	if (!remotePeerID.IsValid())
	{
		UE_LOG_TRAFFIC(Error, TEXT("Invalid remote PeerID: peerID=%s, URL=%s"), *remotePeerID.ToString(), *URL.ToString(true));
		return;
	}

	if (Handler.IsValid() && !Handler->GetRawSend())
	{
#if ENGINE_MINOR_VERSION >= 21
		const auto processedDataPacket = Handler->Outgoing(dataToSend, InCountBits, OutTraits);
#else
		const auto processedDataPacket = Handler->Outgoing(dataToSend, InCountBits);
#endif
		if (processedDataPacket.bError)
		{
			UE_LOG_TRAFFIC(Error, TEXT("Error processing packet with connectionless handler"));
			return;
		}

		dataToSend = processedDataPacket.Data;
		InCountBits = processedDataPacket.CountBits;
	}

	auto bytesToSend = FMath::DivideAndRoundUp(InCountBits, 8);

#if ENGINE_MINOR_VERSION >= 18 && !UE_BUILD_SHIPPING
	bool bBlockSend = false;
	LowLevelSendDel.ExecuteIfBound(InData, bytesToSend, bBlockSend);
	if (bBlockSend)
	{
		UE_LOG_TRAFFIC(Log, TEXT("Packet sending is blocked"));
		return;
	}
#endif

	if (bytesToSend <= 0)
	{
		UE_LOG_TRAFFIC(Log, TEXT("No data to send. Skipping"));
		return;
	}

	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetConnectionGOG::SendP2PPacket: remote='%s'; size='%d' bytes; data={%s}"),
		*LowLevelGetRemoteAddress(), bytesToSend, *BytesToHex(dataToSend, bytesToSend));

	galaxy::api::Networking()->SendP2PPacket(
		remotePeerID,
		dataToSend,
		bytesToSend,
#if ENGINE_MINOR_VERSION >= 25
		IsInternalAck()
#else
		InternalAck
#endif
			? galaxy::api::P2P_SEND_RELIABLE_IMMEDIATE
			: galaxy::api::P2P_SEND_UNRELIABLE_IMMEDIATE);

	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_TRAFFIC(Error, TEXT("Failed to send data: remote='%s'; %s; %s"), *LowLevelGetRemoteAddress(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
}

FString UNetConnectionGOG::LowLevelGetRemoteAddress(bool InAppendPort)
{
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetConnectionGOG::LowLevelGetRemoteAddress()"));

	return URL.ToString(InAppendPort);
}

FString UNetConnectionGOG::LowLevelDescribe()
{
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetConnectionGOG::LowLevelDescribe()"));

	return LowLevelGetRemoteAddress();
}

FString UNetConnectionGOG::RemoteAddressToString()
{
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetConnectionGOG::RemoteAddressToString()"));

	return LowLevelGetRemoteAddress(/* bAppendPort */ true);
}

UNetConnectionGOG::LobbyMemberStateListener::LobbyMemberStateListener(UNetConnectionGOG& InConnection)
	: connection{InConnection}
{
	check(connection.Driver != nullptr);
	check(connection.Driver->IsServer() && "LobbyMemberStateChanged should only be handled by the Server");
}

void UNetConnectionGOG::LobbyMemberStateListener::OnLobbyMemberStateChanged(const galaxy::api::GalaxyID& InLobbyID, const galaxy::api::GalaxyID& InMemberID, galaxy::api::LobbyMemberStateChange InMemberStateChange)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetConnectionGOG::OnLobbyMemberStateChanged()"));

	if (InMemberID != connection.remotePeerID)
	{
		UE_LOG_NETWORKING(Verbose, TEXT("Lobby member from another connection. Ignoring"));
		return;
	}

	if (InMemberStateChange == galaxy::api::LOBBY_MEMBER_STATE_CHANGED_LEFT
		|| InMemberStateChange == galaxy::api::LOBBY_MEMBER_STATE_CHANGED_DISCONNECTED
		|| InMemberStateChange == galaxy::api::LOBBY_MEMBER_STATE_CHANGED_KICKED
		|| InMemberStateChange == galaxy::api::LOBBY_MEMBER_STATE_CHANGED_BANNED)
	{
		// Unregister listener as connection object destruction is delayed until garbage collection
		galaxy::api::ListenerRegistrar()->Unregister(GetListenerType(), this);

		// Closing the connection will eventually destroy a session tied player controller is in
		connection.Close();
	}
}
