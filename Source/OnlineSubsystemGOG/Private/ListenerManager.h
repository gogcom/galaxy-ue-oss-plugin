#pragma once

#include "Types/IListenerGOG.h"

class FListenerManager
{
public:

	void FreeListener(FSetElementId InListenerID)
	{
		listenerRegistry.Remove(MoveTemp(InListenerID));
	}

	template<class Listener, typename... Args>
	TPair<FSetElementId, Listener*> CreateListener(Args&&... args)
	{
		auto listenerID = listenerRegistry.Emplace(
#if ENGINE_MINOR_VERSION == 24
			MakeShared<Listener>(Forward<Args>(args)...)
#else
			MakeUnique<Listener>(Forward<Args>(args)...)
#endif
		);
		const auto& listener = listenerRegistry[listenerID];
		listener->ListenerID = listenerID;

		return MakeTuple(MoveTemp(listenerID), static_cast<Listener*>(listener.Get()));
	}

private:
#if ENGINE_MINOR_VERSION == 24
	// TSet works only on copyable elements in 4.24
	TSet<TSharedPtr<IListenerGOG>> listenerRegistry;
#else
	TSet<TUniquePtr<IListenerGOG>> listenerRegistry;
#endif
};