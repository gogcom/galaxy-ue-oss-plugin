#pragma once

#include "CommonGOG.h"
#include "NetConnectionGOG.h"

#include "Engine/NetDriver.h"
#include <array>

#include "NetDriverGOG.generated.h"

constexpr uint32_t PREALLOCATED_BUFFER_SIZE = 4096;

UCLASS(transient, config = Engine)
class UNetDriverGOG
	: public UNetDriver
{

	GENERATED_BODY()

public:

	bool IsAvailable() const override;

	bool IsNetResourceValid() override;

	bool InitBase(bool InInitAsClient, FNetworkNotify* InNotify, const FURL& InURL, bool InReuseAddressAndPort, FString& OutError) override;

	bool InitConnect(FNetworkNotify* InNotify, const FURL& InConnectURL, FString& OutError) override;

	bool InitListen(FNetworkNotify* InNotify, FURL& InLocalURL, bool InReuseAddressAndPort, FString& OutError) override;

	void TickDispatch(float InDeltaTime) override;

#if ENGINE_MINOR_VERSION >= 23 || ENGINE_MAJOR_VERSION > 4
	void LowLevelSend(TSharedPtr<const FInternetAddr> InAddress, void* InData, int32 InCountBits, FOutPacketTraits& OutTraits) override;
#elif ENGINE_MINOR_VERSION >= 21
	void LowLevelSend(FString InAddress, void* InData, int32 InCountBits, FOutPacketTraits& OutTraits) override;
#else
	void LowLevelSend(FString InAddress, void* InData, int32 InCountBits) override;
#endif

	FString LowLevelGetNetworkNumber() override;

	class ISocketSubsystem* GetSocketSubsystem() override;

private:

	bool ChallengeConnectingClient(const FUniqueNetIdGOG& InSenderID, uint8* InOutData, uint32_t& InOutDataSize);

	UNetConnectionGOG* EstablishIncomingConnection(const FUniqueNetIdGOG& InSenderID);

	UNetConnection* FindEstablishedConnection(const FUniqueNetIdGOG& InSenderID) const;

	class LobbyLeftListener
		: public galaxy::api::GlobalLobbyLeftListener
	{
	public:

		LobbyLeftListener(UNetDriverGOG& InDriver);

		void OnLobbyLeft(const galaxy::api::GalaxyID& InLobbyID, galaxy::api::ILobbyLeftListener::LobbyLeaveReason InLeaveReason) override;

		UNetDriverGOG& driver;
	};

	TUniquePtr<LobbyLeftListener> lobbyLeftListener;

	galaxy::api::GalaxyID serverUserId;

	std::array<uint8, PREALLOCATED_BUFFER_SIZE> inPacketBuffer;
	uint8* receivedPackedData{nullptr};
};
