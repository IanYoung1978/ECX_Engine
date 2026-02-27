#include "ECXEventBroker.h"
#include "IEventListener.h"

ECXEventBroker::ECXEventBroker()
{
	dequeSelector = 0;
}

void ECXEventBroker::Subscribe(IEventListener& listener, ECXEventType type)
{
	listeners[type].push_back(&listener);
}

void ECXEventBroker::publish(ECXEvent& Event)
{
	std::scoped_lock<std::mutex> lock(pushLock);
	queues[dequeSelector].push_back(Event);
}

void ECXEventBroker::flush()
{
	dequeSelector = (dequeSelector == 0) ? 1 : 0;
	while (!queues[!dequeSelector].empty())
	{
		auto& listenerSet = listeners[queues[!dequeSelector].front().type];
		if (listenerSet.size() > 0)
		{
			auto Event = queues[!dequeSelector].front();
			queues[!dequeSelector].pop_front();
			for (auto listener : listenerSet)
			{
				listener->receive(Event);
			}
		}
		
	}
}

void ECXEventBroker::clear()
{
	std::scoped_lock<std::mutex> lock(pushLock);
	queues[0].clear();
	queues[1].clear();
}
