#pragma once

#include "OnlineSubsystem.h"
#include "OnlineSubsystemGOG.h"

class FOnlineFactoryGOG : public IOnlineFactory
{
public:

	FOnlineFactoryGOG();

	~FOnlineFactoryGOG() override;

	IOnlineSubsystemPtr CreateSubsystem(FName InInstanceName) override;

private:

	void DestroyOnlineSubsystemGOG();

	FOnlineSubsystemGOGPtr onlineSubsystemGOG;
};
