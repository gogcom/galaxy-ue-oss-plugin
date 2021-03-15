#pragma once

#include "CommonGOG.h"
#include "Templates/UniquePtr.h"
#include "Containers/Set.h"

// Interface needed to play-around lack of virtual inheritance in IGalaxy listeners
class IListenerGOG
{
public:

	virtual ~IListenerGOG() = default;

	FSetElementId ListenerID;
};

template<class GalaxyListener>
class SelfDeregisteringListenerGOG : public GalaxyListener
{
public:

	virtual ~SelfDeregisteringListenerGOG()
	{
		galaxy::api::ListenerRegistrar()->Unregister(GalaxyListener::GetListenerType(), this);
	}
};

inline size_t GetTypeHash(const TUniquePtr<IListenerGOG>& inListener)
{
	return reinterpret_cast<size_t>(inListener.Get());
}
