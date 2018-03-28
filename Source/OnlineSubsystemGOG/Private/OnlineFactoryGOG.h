#pragma once

#include "OnlineSubsystem.h"

class FOnlineFactoryGOG : public IOnlineFactory
{
public:

	FOnlineFactoryGOG();

	virtual ~FOnlineFactoryGOG();

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InInstanceName) override;
};