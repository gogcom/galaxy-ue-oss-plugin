#include "NetDriverGOG.h"
#include "Network/InternetAddrGOG.h"
#include "Types/UniqueNetIdGOG.h"
#include "Loggers.h"

#include "PacketHandlers/StatelessConnectHandlerComponent.h"
#include "Engine/Engine.h"
#include "Engine/ChildConnection.h"

#include <vector>
#include <algorithm>
#include <iterator>
#include <xutility>


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
	UE_LOG_NETWORKING(VeryVerbose, TEXT("UNetDriverGOG::IsAvailable()"));

	return IOnlineSubsystem::Get(TEXT_GOG) != nullptr;
}

bool UNetDriverGOG::IsNetResourceValid()
{
	UE_LOG_NETWORKING(VeryVerbose, TEXT("UNetDriverGOG::IsNetResourceValid()"));

	if (!IsAvailable())
		return false;

	if (IsServer())
		return true;

	// Clients should not send packets to closed server connection
	return ServerConnection->State == EConnectionState::USOCK_Open;
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

	SetParameterIfZero(InitialConnectTimeout, "InitalConnectTimeout", 10.f);
	SetParameterIfZero(ConnectionTimeout, "ConnectionTimeout", 5.f);
	SetParameterIfZero(KeepAliveTime, "KeepAliveTime", 0.f);
	SetParameterIfZero(SpawnPrioritySeconds, "SpawnPrioritySeconds", 1.f);
	SetParameterIfZero(RelevantTimeout, "RelevantTimeout", 5.f);
	SetParameterIfZero(ServerTravelPause, "ServerTravelPause", 5.f);

	lobbyLeftListener.Reset(new LobbyLeftListener{*this});

	return true;
}

bool UNetDriverGOG::InitConnect(FNetworkNotify* InNotify, const FURL& InConnectURL, FString& InOutError)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::InitConnect()"));

	galaxyNetworking = galaxy::api::Networking();
	check(galaxyNetworking);

	if (!InitBase(true, InNotify, InConnectURL, false, InOutError))
		return false;

	check(!ServerConnection);

	ServerConnection = NewObject<UNetConnectionGOG>(NetConnectionClass);
	check(ServerConnection);

	ServerConnection->InitLocalConnection(this, nullptr, InConnectURL, USOCK_Open);

	ServerConnection->CreateChannel(CHTYPE_Control, true, INDEX_NONE);

	return true;
}

bool UNetDriverGOG::InitListen(FNetworkNotify* InNotify, FURL& InLocalURL, bool InReuseAddressAndPort, FString& OutError)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::InitListen()"));

	galaxyNetworking = galaxy::api::ServerNetworking();
	check(galaxyNetworking);

	if (!InitBase(false, InNotify, InLocalURL, InReuseAddressAndPort, OutError))
		return false;

	InitConnectionlessHandler();

	return true;
}

void UNetDriverGOG::ProcessRemoteFunction(class AActor* InActor, class UFunction* InFunction, void* InParameters, struct FOutParmRec* OutParms, struct FFrame* InStack, class UObject* InSubObject /*= NULL*/)
{
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::ProcessRemoteFunction()"));

#if !UE_BUILD_SHIPPING
	bool outBlockSendRPC = false;

	SendRPCDel.ExecuteIfBound(InActor, InFunction, InParameters, OutParms, InStack, InSubObject, outBlockSendRPC);

	if (outBlockSendRPC)
	{
		UE_LOG_TRAFFIC(Log, TEXT("RPC call blocked. Ignoring"));
		return;
	}
#endif

	if (IsServer() && (InFunction->FunctionFlags & FUNC_NetMulticast))
	{
		// Multicast functions go to every client. Works only from server

		for (auto clientConnection : ClientConnections)
		{
			if (!clientConnection)
			{
				UE_LOG_TRAFFIC(Warning, TEXT("Invalid ClientConnection when trying to multicast RPC. Ignoring"));
				continue;
			}

			// Do relevancy check if unreliable.
			// Reliables will always go out. This is odd behavior. On one hand we wish to guarantee "reliables always get there". On the other
			// hand, replicating a reliable to something on the other side of the map that is non relevant seems weird.
			//
			// Multicast reliables should probably never be used in gameplay code for actors that have relevancy checks. If they are, the
			// RPC will go through and the channel will be closed soon after due to relevancy failing.

			bool isRelevant = true;
			if ((InFunction->FunctionFlags & FUNC_NetReliable) == 0)
			{
				if (clientConnection->ViewTarget)
				{
					FNetViewer netViewer{clientConnection, 0.f};
					isRelevant = InActor->IsNetRelevantFor(netViewer.InViewer, netViewer.ViewTarget, netViewer.ViewLocation);
				}
				else
				{
					// No netViewer for this clientConnection(?), just let it go through.
					UE_LOG_TRAFFIC(Log, TEXT("Multicast function called on an actor when a ClientConnection has no NetViewer: functionName='%s', actorName='%s'"), *InFunction->GetName(), *InActor->GetName());
				}
			}

			if (!isRelevant)
			{
				UE_LOG_TRAFFIC(Verbose, TEXT("RPC is irrelevant for ClientConnection. Skipping: functionName='%s', actorName='%s'"), *InFunction->GetName(), *InActor->GetName());
				continue;
			}

			if (clientConnection->GetUChildConnection() != nullptr)
				clientConnection = ((UChildConnection*)clientConnection)->Parent;

			InternalProcessRemoteFunction(InActor, InSubObject, clientConnection, InFunction, InParameters, OutParms, InStack, true);
		}
	}
	else
	{
		auto netConnection = InActor->GetNetConnection();
		if (!netConnection)
		{
			UE_LOG_TRAFFIC(Warning, TEXT("Trying to call RPC on an actor without ClientConnection. Ignoring"));
			return;
		}

		InternalProcessRemoteFunction(InActor, InSubObject, netConnection, InFunction, InParameters, OutParms, InStack, IsServer());
	}
}

void UNetDriverGOG::TickDispatch(float InDeltaTime)
{
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::TickDispatch()"));

	UNetDriver::TickDispatch(InDeltaTime);

	check(galaxyNetworking && "Galaxy Networking interface is NULL");

	uint32_t expectedPacketSize;
	uint32_t actualPacketSize;

	galaxy::api::GalaxyID senderGalaxyID;

	const galaxy::api::IError* err{nullptr};
	auto isP2PPacketAvailable = true;

	while (true)
	{
		isP2PPacketAvailable = galaxyNetworking->IsP2PPacketAvailable(&expectedPacketSize);
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

		galaxyNetworking->ReadP2PPacket(inPacketBuffer.data(), inPacketBuffer.size(), &actualPacketSize, senderGalaxyID);
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

		UNetConnection* connection{nullptr};
		if (IsServer())
		{
			connection = FindEstablishedConnection(senderID);
			if (!connection)
			{
				if (!ChallengeConnectingClient(senderID, inPacketBuffer.data(), actualPacketSize))
					return;

				connection = EstablishIncomingConnection(senderID);
			}
		}
		else
		{
			if (AsNetConnectionGOG(ServerConnection)->remotePeerID != senderID)
			{
				UE_LOG_TRAFFIC(Error, TEXT("Client recieved packet from an unknown source: serverID='%s', packetSenderID='%s'"),
					*AsNetConnectionGOG(ServerConnection)->remotePeerID.ToString(), *senderID.ToString());
				continue;
			}
			check(ServerConnection && "Received a packet yet server connection is NULL");
			connection = ServerConnection;
		}

		if (!connection)
		{
			UE_LOG_TRAFFIC(Error, TEXT("Cannot receive packet; null connection: senderID='%s'"), *senderID.ToString());
			continue;
		}

		connection->ReceivedRawPacket(inPacketBuffer.data(), actualPacketSize);
	}
}

bool UNetDriverGOG::ChallengeConnectingClient(const FUniqueNetIdGOG& InSenderID, void* InData, uint32_t InDataSize)
{
	auto incomingAddress = FInternetAddrGOG{InSenderID}.ToString(true);

	if (Notify->NotifyAcceptingConnection() != EAcceptConnection::Accept)
	{
		UE_LOG_NETWORKING(Warning, TEXT("New incoming connections denied: senderID='%s'"), *incomingAddress);
		return false;
	}

	if (!ConnectionlessHandler.IsValid())
	{
		UE_LOG_NETWORKING(Warning, TEXT("Handshake challenge is in progress, yet ConnectionlessHandler is invalid: remoteID='%s'"), *incomingAddress);
		return false;
	}

	auto dataToSend = reinterpret_cast<uint8*>(InData);

	const auto unprocessedPacket = ConnectionlessHandler->IncomingConnectionless(incomingAddress, dataToSend, InDataSize);
	if (unprocessedPacket.bError)
	{
		UE_LOG_NETWORKING(Warning, TEXT("Failed to process incoming handshake packet: remoteID='%s'"), *incomingAddress);
		return false;
	}

	auto statelessHandshakeComponent = StatelessConnectComponent.Pin();
	if (!statelessHandshakeComponent.IsValid())
	{
		UE_LOG_NETWORKING(Warning, TEXT("Handshake challenge is in progress, yet StatelessHandshakeComponent is invalid: remoteID='%s'"), *incomingAddress);
		return false;
	}

	if (!statelessHandshakeComponent->HasPassedChallenge(incomingAddress))
	{
		UE_LOG_NETWORKING(Warning, TEXT("Client failed handshake challenge: remoteID='%s'"), *incomingAddress);
		return false;
	}

	UE_LOG_NETWORKING(Log, TEXT("Client passed handshake challenge: remoteID='%s'"), *incomingAddress);
	return true;
}

UNetConnectionGOG* UNetDriverGOG::EstablishIncomingConnection(const FUniqueNetIdGOG& InSenderID)
{
	auto statelessHandshakeHandler = StatelessConnectComponent.Pin();
	if (!statelessHandshakeHandler.IsValid())
	{
		UE_LOG_NETWORKING(Log, TEXT("Stateless connection handshake handler is invalid. Cannot accept connection: remoteID='%s'"), *InSenderID.ToString());
		return nullptr;
	}

	auto newIncomingConnection = NewObject<UNetConnectionGOG>(NetConnectionClass);
	check(newIncomingConnection);

	// Set the initial packet sequence from the handshake data

	int32 serverSequence = 0;
	int32 clientSequence = 0;
	statelessHandshakeHandler->GetChallengeSequence(serverSequence, clientSequence);
	newIncomingConnection->InitSequence(clientSequence, serverSequence);

	newIncomingConnection->InitRemoteConnection(this, nullptr, FURL(), FInternetAddrGOG{InSenderID}, USOCK_Open);

	if (newIncomingConnection->Handler.IsValid())
		newIncomingConnection->Handler->BeginHandshaking();

	Notify->NotifyAcceptedConnection(newIncomingConnection);
	AddClientConnection(newIncomingConnection);

	return newIncomingConnection;
}

UNetConnection* UNetDriverGOG::FindEstablishedConnection(const FUniqueNetIdGOG& senderID)
{
	auto storedConnectionIt = ClientConnections.FindByPredicate([&](auto storedConnection) {
		return AsNetConnectionGOG(storedConnection)->remotePeerID == senderID;
	});

	if (!storedConnectionIt)
		return nullptr;

	return *storedConnectionIt;
}

void UNetDriverGOG::LowLevelSend(FString InAddress, void* InData, int32 InCountBits)
{
	UE_LOG_TRAFFIC(VeryVerbose, TEXT("UNetDriverGOG::LowLevelSend()"));

	auto remoteID = FUniqueNetIdGOG{InAddress};
	if (!remoteID.IsValid())
	{
		UE_LOG_TRAFFIC(Error, TEXT("Failed to parse GOG PeerID from send address: '%s'"), *InAddress);
		return;
	}

	auto connection = FindEstablishedConnection(remoteID);
	if (connection == nullptr)
		UE_LOG_TRAFFIC(Error, TEXT("Trying to send packet to an unknown peer: remotePeerID='%s'"), *remoteID.ToString());

	auto dataToSend = reinterpret_cast<uint8*>(InData);

	if (ConnectionlessHandler.IsValid())
	{
		const ProcessedPacket processedDataPacket = ConnectionlessHandler->OutgoingConnectionless(InAddress, dataToSend, InCountBits);

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

	UE_LOG_TRAFFIC(VeryVerbose, TEXT("Low level send: remoteID='%s'; dataSize='%d' bytes"), *remoteID.ToString(), bytesToSend);

	galaxyNetworking->SendP2PPacket(remoteID, dataToSend, bytesToSend, galaxy::api::P2P_SEND_UNRELIABLE_IMMEDIATE);

	auto err = galaxy::api::GetError();
	if (err)
		UE_LOG_TRAFFIC(Error, TEXT("Failed to send packet: remoteID='%s'; %s; %s"), *remoteID.ToString(), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
}

FString UNetDriverGOG::LowLevelGetNetworkNumber()
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::LowLevelGetNetworkNumber()"));

	auto galaxyID = galaxy::api::User()->GetGalaxyID();
	auto err = galaxy::api::GetError();
	if (err || !galaxyID.IsValid())
	{
		UE_LOG_NETWORKING(Error, TEXT("Failed to get GOG UserId: %s; %s"), UTF8_TO_TCHAR(err->GetName()), UTF8_TO_TCHAR(err->GetMsg()));
		return{};
	}

	return FInternetAddrGOG{galaxyID}.ToString(true);
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

void UNetDriverGOG::LobbyLeftListener::OnLobbyLeft(const galaxy::api::GalaxyID& InLobbyID, bool InIOFailure)
{
	UE_LOG_NETWORKING(Log, TEXT("UNetDriverGOG::OnLobbyMemberStateChanged()"));

	if (driver.IsServer())
	{
		// TODO: neither NetDriver::Shutdown() nor OnDestroySessionCompleteDelegates() are closing
		// lobby in a test game when OnLobbyLeft() is suddenly called. Did we forgot some delegate?
		driver.Shutdown();
	}
	else
	{
		if (InLobbyID != AsNetConnectionGOG(driver.ServerConnection)->remotePeerID)
		{
			UE_LOG_NETWORKING(Log, TEXT("Unknown lobby. Skipping"));
			return;
		}

		driver.ServerConnection->Close();
	}

	galaxy::api::ListenerRegistrar()->Unregister(GetListenerType(), this);
}