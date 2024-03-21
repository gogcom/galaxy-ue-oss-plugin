#include "NetDriverGOG.h"
#include "InternetAddrGOG.h"
#include "Types/UrlGOG.h"
#include "Types/UniqueNetIdGOG.h"
#include "Loggers.h"
#include "UserInfoUtils.h"

#include "PacketHandlers/StatelessConnectHandlerComponent.h"
#include "Engine/Engine.h"
#include "Engine/ChildConnection.h"
#include "SocketSubsystem.h"

#include <vector>
#include <algorithm>
#include <iterator>

namespace
{

	inline void SetParameterIfZero(float& InParam, const char* InParamName, float InInitialValue)
	{
		if (InParam == 0.0)
		{
			UE_LOG_NETWORKING(Log, TEXT("%s is reset to %f"), UTF8_TO_TCHAR(InParamName), InInitialValue);
			InParam = InInitialValue;
		}
	}

}

bool UNetDriverGOG::IsAvailable() const
{
	return IOnlineSubsystem::Get(TEXT_GOG) != nullptr;
}

bool UNetDriverGOG::IsNetResourceValid()
{
	if (!IsAvailable())
		return false;

	if (IsServer())
		return true;

	// Clients should not send packets to closed server connection
#if ENGINE_MAJOR_VERSION > 4
	return ServerConnection->GetConnectionState() == EConnectionState::USOCK_Open;
#else
	return ServerConnection->State == EConnectionState::USOCK_Open;
#endif
}

bool UNetDriverGOG::InitBase(bool InInitAsClient, FNetworkNotify* InNotify, const FURL& InURL, bool InReuseAddressAndPort, FString& OutError)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::InitBase()"));

#if !UE_BUILD_SHIPPING
	bNoTimeouts = true;
#endif

	if (!UNetDriver::InitBase(InInitAsClient, InNotify, InURL, InReuseAddressAndPort, OutError))
	{
		UE_LOG_NETWORKING(Error, TEXT("Failed to initialized NetDriver base"));
		return false;
	}

	SetParameterIfZero(InitialConnectTimeout, "InitalConnectTimeout", 120.f);
	SetParameterIfZero(ConnectionTimeout, "ConnectionTimeout", 120.f);
	SetParameterIfZero(KeepAliveTime, "KeepAliveTime", 0.2f);
	SetParameterIfZero(SpawnPrioritySeconds, "SpawnPrioritySeconds", 1.f);
	SetParameterIfZero(RelevantTimeout, "RelevantTimeout", 5.f);
	SetParameterIfZero(ServerTravelPause, "ServerTravelPause", 5.f);

	lobbyLeftListener.Reset(new LobbyLeftListener{*this});

	return true;
}

bool UNetDriverGOG::InitConnect(FNetworkNotify* InNotify, const FURL& InConnectURL, FString& InOutError)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::InitConnect(URL=%s)"), *InConnectURL.ToString(true));

	if (!InitBase(true, InNotify, InConnectURL, false, InOutError))
		return false;

	check(!ServerConnection && !serverUserId.IsValid());

	ServerConnection = NewObject<UNetConnectionGOG>(NetConnectionClass);
	check(ServerConnection);

	serverUserId = galaxy::api::Matchmaking()->GetLobbyOwner(FUniqueNetIdGOG{InConnectURL.Host});
	if (!serverUserId.IsValid() || serverUserId.GetIDType() != galaxy::api::GalaxyID::ID_TYPE_USER)
	{
		UE_LOG_NETWORKING(Error, TEXT("Failed to connect to host. Invalid user id: connectUrlHost='%s', hostID=%llu"),
			*InConnectURL.Host, serverUserId.ToUint64());
		return false;
	}

	ServerConnection->InitLocalConnection(this, /*no socket*/ nullptr, InConnectURL, USOCK_Open);

#if ENGINE_MINOR_VERSION >= 22 || ENGINE_MAJOR_VERSION > 4
	CreateInitialClientChannels();
#else
	ServerConnection->CreateChannel(CHTYPE_Control, true, INDEX_NONE);
#endif

	return true;
}

bool UNetDriverGOG::InitListen(FNetworkNotify* InNotify, FURL& InLocalURL, bool InReuseAddressAndPort, FString& OutError)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::InitListen(URL=%s)"), *InLocalURL.ToString(true));

	if (!InitBase(false, InNotify, InLocalURL, InReuseAddressAndPort, OutError))
		return false;

	InitConnectionlessHandler();

#if ENGINE_MINOR_VERSION >= 23 || ENGINE_MAJOR_VERSION > 4
	auto ownUserID = UserInfoUtils::GetOwnUserID();
	if (!ownUserID.IsValid())
		return false;

	LocalAddr = MakeShared<FInternetAddrGOG>(MoveTemp(ownUserID));
#endif

	return true;
}

void UNetDriverGOG::TickDispatch(float InDeltaTime)
{
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::TickDispatch()"));

	UNetDriver::TickDispatch(InDeltaTime);

	uint32_t expectedPacketSize;
	uint32_t actualPacketSize;

	galaxy::api::GalaxyID senderGalaxyID;

	const galaxy::api::IError* err{nullptr};
	auto isP2PPacketAvailable = true;

	while (true)
	{
		isP2PPacketAvailable = galaxy::api::Networking()->IsP2PPacketAvailable(&expectedPacketSize);
		err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_TRAFFIC(Warning, TEXT("Failed to check for packet availability: '%s'; '%s'"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			return;
		}

		if (!isP2PPacketAvailable)
		{
			UE_LOG_TRAFFIC(VeryVerbose, TEXT("No packets to read"));
			return;
		}

		galaxy::api::Networking()->ReadP2PPacket(inPacketBuffer.data(), inPacketBuffer.size(), &actualPacketSize, senderGalaxyID);
		UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::ReadP2PPacket: size=%u bytes; data={%s}"),
			actualPacketSize, *BytesToHex(inPacketBuffer.data(), actualPacketSize));

		err = galaxy::api::GetError();
		if (err)
		{
			UE_LOG_TRAFFIC(Error, TEXT("Failed to read incoming packet: '%s'; '%s'"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
			continue;
		}

		if (!senderGalaxyID.IsValid())
		{
			UE_LOG_TRAFFIC(Error, TEXT("SenderID is invalid"));
			continue;
		}

		if (actualPacketSize != expectedPacketSize)
			UE_LOG_TRAFFIC(Warning, TEXT("Actual packet size is different from expected packet size: expected='%u', actual='%u'"), expectedPacketSize, actualPacketSize);

		auto senderID = FUniqueNetIdGOG{senderGalaxyID};
		receivedPackedData = inPacketBuffer.data();

		UNetConnection* connection{nullptr};
		if (IsServer())
		{
			connection = FindEstablishedConnection(senderID);
			if (!connection)
			{
				UE_LOG_TRAFFIC(VeryVerbose, TEXT("No established connection yet : senderID='%s'"), *senderID.ToString());

				if (!ChallengeConnectingClient(senderID, receivedPackedData, actualPacketSize))
					continue;

				connection = EstablishIncomingConnection(senderID);
			}
		}
		else
		{
			if (senderGalaxyID != serverUserId)
			{
				UE_LOG_TRAFFIC(Error, TEXT("Client recieved packet from an unknown source: serverAddress='%s', senderID='%s'"),
					*ServerConnection->RemoteAddressToString(), *senderID.ToString());
				continue;
			}

			if (!ServerConnection)
			{
				UE_LOG_TRAFFIC(Error, TEXT("Client received a packet yet server connection is NULL"));
				continue;
			}

			connection = ServerConnection;
		}

		if (!connection)
		{
			UE_LOG_TRAFFIC(Error, TEXT("Cannot receive packet; null connection: senderID='%s'"), *senderID.ToString());
			continue;
		}

		UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::ReceivedRawPacket: size=%u bytes; data={%s}"), actualPacketSize, *BytesToHex(receivedPackedData, actualPacketSize));
		if (!actualPacketSize || !receivedPackedData)
		{
			UE_LOG_TRAFFIC(VeryVerbose, TEXT("No data left to process in packet: senderID='%s'"), *senderID.ToString());
			continue;
		}

		connection->ReceivedRawPacket(receivedPackedData, actualPacketSize);
	}
}

bool UNetDriverGOG::ChallengeConnectingClient(const FUniqueNetIdGOG& InSenderID, uint8* InOutData, uint32_t& InOutDataSize)
{
	if (Notify->NotifyAcceptingConnection() != EAcceptConnection::Accept)
	{
		UE_LOG_NETWORKING(Warning, TEXT("New incoming connections denied: senderID='%s'"), *InSenderID.ToString());
		return false;
	}

	if (!ConnectionlessHandler.IsValid())
	{
		UE_LOG_NETWORKING(Warning, TEXT("Handshake challenge is in progress, yet ConnectionlessHandler is invalid: senderID='%s'"), *InSenderID.ToString());
		return false;
	}

#if ENGINE_MINOR_VERSION >= 23 || ENGINE_MAJOR_VERSION > 4
	auto remoteAddress = MakeShared<FInternetAddrGOG>(InSenderID);
#else
	auto remoteAddress = FUrlGOG{InSenderID}.ToString();
#endif

	const auto unprocessedPacket = ConnectionlessHandler->IncomingConnectionless(remoteAddress, InOutData, InOutDataSize);
	if (unprocessedPacket.bError)
	{
		UE_LOG_NETWORKING(Warning, TEXT("Failed to process incoming handshake packet: senderID='%s'"), *InSenderID.ToString());
		return false;
	}

	InOutDataSize = FMath::DivideAndRoundUp(unprocessedPacket.CountBits, 8);
	InOutData = InOutDataSize > 0 ? unprocessedPacket.Data : nullptr;

	auto statelessHandshakeComponent = StatelessConnectComponent.Pin();
	if (!statelessHandshakeComponent.IsValid())
	{
		UE_LOG_NETWORKING(Warning, TEXT("Handshake challenge is in progress, yet StatelessHandshakeComponent is invalid: senderID='%s'"), *InSenderID.ToString());
		return false;
	}

#if ENGINE_MINOR_VERSION >= 22 || ENGINE_MAJOR_VERSION > 4
	bool isHandshakeRestarted{false};
	if (!statelessHandshakeComponent->HasPassedChallenge(remoteAddress, isHandshakeRestarted))
#else
	if (!statelessHandshakeComponent->HasPassedChallenge(remoteAddress))
#endif
	{
		UE_LOG_NETWORKING(Warning, TEXT("Client failed handshake challenge: senderID='%s'"), *InSenderID.ToString());
		return false;
	}

	UE_LOG_NETWORKING(Log, TEXT("Client passed handshake challenge: senderID='%s'"), *InSenderID.ToString());
	return true;
}

UNetConnectionGOG* UNetDriverGOG::EstablishIncomingConnection(const FUniqueNetIdGOG& InSenderID)
{
	auto statelessHandshakeHandler = StatelessConnectComponent.Pin();
	if (!statelessHandshakeHandler.IsValid())
	{
		UE_LOG_NETWORKING(Log, TEXT("Stateless connection handshake handler is invalid. Cannot accept connection: senderID='%s'"), *InSenderID.ToString());
		return nullptr;
	}

	auto newIncomingConnection = NewObject<UNetConnectionGOG>(NetConnectionClass);
	check(newIncomingConnection);

	newIncomingConnection->InitRemoteConnection(this, nullptr, FUrlGOG{InSenderID}, FInternetAddrGOG{InSenderID}, USOCK_Open);
	AddClientConnection(newIncomingConnection);

#if ENGINE_MINOR_VERSION >= 17 || ENGINE_MAJOR_VERSION > 4
	// Set the initial packet sequence from the handshake data
	int32 serverSequence = 0;
	int32 clientSequence = 0;

	statelessHandshakeHandler->GetChallengeSequence(serverSequence, clientSequence);

	newIncomingConnection->InitSequence(clientSequence, serverSequence);
#if ENGINE_MINOR_VERSION >= 24 || ENGINE_MAJOR_VERSION > 4
	statelessHandshakeHandler->ResetChallengeData();
#endif

	if (newIncomingConnection->Handler.IsValid())
		newIncomingConnection->Handler->BeginHandshaking();
#endif

	Notify->NotifyAcceptedConnection(newIncomingConnection);

	return newIncomingConnection;
}

UNetConnection* UNetDriverGOG::FindEstablishedConnection(const FUniqueNetIdGOG& InRemoteUniqueID) const
{
	auto remoteUrl = FUrlGOG{InRemoteUniqueID};

	auto storedConnectionIt = ClientConnections.FindByPredicate([&](const auto& storedConnection) {
		return remoteUrl == storedConnection->URL;
	});

	if (!storedConnectionIt)
		return nullptr;

	return *storedConnectionIt;
}

#if ENGINE_MINOR_VERSION >= 23 || ENGINE_MAJOR_VERSION > 4
void UNetDriverGOG::LowLevelSend(TSharedPtr<const FInternetAddr> InAddress, void* InData, int32 InCountBits, FOutPacketTraits& OutTraits)
#elif ENGINE_MINOR_VERSION >= 21
void UNetDriverGOG::LowLevelSend(FString InAddress, void* InData, int32 InCountBits, FOutPacketTraits& OutTraits)
#else
void UNetDriverGOG::LowLevelSend(FString InAddress, void* InData, int32 InCountBits)
#endif
{
	auto dataToSend = reinterpret_cast<uint8*>(InData);

#if ENGINE_MINOR_VERSION >= 23 || ENGINE_MAJOR_VERSION > 4
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::LowLevelSend(): address='%s'; size=%u bits; data={%s}"),
		*InAddress->ToString(true), InCountBits, *BytesToHex(dataToSend, FMath::DivideAndRoundUp(InCountBits, 8)));

	const auto rawIP = InAddress->GetRawIp();
	auto remoteID = FUniqueNetIdGOG{rawIP.GetData(), rawIP.Num()};
	if (!remoteID.IsValid())
	{
		UE_LOG_TRAFFIC(Error, TEXT("Failed to parse GOG PeerID from send address: '%s'"), *InAddress->ToString(true));
#else
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::LowLevelSend(): address='%s'; size=%u bits; data={%s}"),
		*InAddress, InCountBits, *BytesToHex(dataToSend, FMath::DivideAndRoundUp(InCountBits, 8)));

	auto remoteID = FUniqueNetIdGOG{InAddress};
	if (!remoteID.IsValid())
	{
		UE_LOG_TRAFFIC(Error, TEXT("Failed to parse GOG PeerID from send address: '%s'"), *InAddress);
#endif
		return;
	}

	if (ConnectionlessHandler.IsValid())
	{
#if ENGINE_MINOR_VERSION >= 21 || ENGINE_MAJOR_VERSION > 4
		const ProcessedPacket processedDataPacket = ConnectionlessHandler->OutgoingConnectionless(InAddress, dataToSend, InCountBits, OutTraits);
#else
		const ProcessedPacket processedDataPacket = ConnectionlessHandler->OutgoingConnectionless(InAddress, dataToSend, InCountBits);
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
	if (bytesToSend <= 0)
	{
		UE_LOG_TRAFFIC(VeryVerbose, TEXT("Nothing to send. Skipping"));
		return;
	}

	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::SendP2PPacket(): remoteID='%s' bytes; size=%d data: {%s}"),
		*remoteID.ToString(), bytesToSend, *BytesToHex(dataToSend, bytesToSend));

	galaxy::api::Networking()->SendP2PPacket(remoteID, dataToSend, bytesToSend, galaxy::api::P2P_SEND_UNRELIABLE_IMMEDIATE);

	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_TRAFFIC(Error, TEXT("Failed to send packet: remoteID='%s'; %s; %s"), *remoteID.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
}

FString UNetDriverGOG::LowLevelGetNetworkNumber()
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::LowLevelGetNetworkNumber()"));

#if ENGINE_MINOR_VERSION >= 23 || ENGINE_MAJOR_VERSION > 4
	if(!LocalAddr)
		return {};

	return LocalAddr->ToString(true);
#else
	return UserInfoUtils::GetOwnUserID().ToString();
#endif
}

ISocketSubsystem* UNetDriverGOG::GetSocketSubsystem()
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::GetSocketSubsystem()"));

	return nullptr;
}

UNetDriverGOG::LobbyLeftListener::LobbyLeftListener(UNetDriverGOG& InDriver)
	: driver{InDriver}
{
}

void UNetDriverGOG::LobbyLeftListener::OnLobbyLeft(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::ILobbyLeftListener::LobbyLeaveReason InLeaveReason)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::OnLobbyLeft()"));

	// TODO: ignore callbacks regarding other lobbies

	// graceful session closing and host leaving shall be handled by the higher levels

	if (InLeaveReason == galaxy::api::ILobbyLeftListener::LOBBY_LEAVE_REASON_USER_LEFT
		|| InLeaveReason == galaxy::api::ILobbyLeftListener::LOBBY_LEAVE_REASON_LOBBY_CLOSED)
		return;

	if (driver.IsServer())
	{
		for (auto clientConnection : driver.ClientConnections)
#if ENGINE_MAJOR_VERSION > 4
			clientConnection->SetConnectionState(EConnectionState::USOCK_Closed);
#else
			clientConnection->State = EConnectionState::USOCK_Closed;
#endif
	}
	else
	{
		if (FUrlGOG{InLobbyID} != driver.ServerConnection->URL)
		{
			UE_LOG_NETWORKING(Log, TEXT("Unknown lobby. Skipping"));
			return;
		}

		driver.serverUserId = galaxy::api::GalaxyID::UNASSIGNED_VALUE;
#if ENGINE_MAJOR_VERSION > 4
		driver.ServerConnection->SetConnectionState(EConnectionState::USOCK_Closed);
#else
		driver.ServerConnection->State = EConnectionState::USOCK_Closed;
#endif
	}

	galaxy::api::ListenerRegistrar()->Unregister(GetListenerType(), this);
}
