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
	// Swapped under pushLock so a publisher can never push into the queue being drained below.
	int draining;
	{
		std::scoped_lock<std::mutex> lock(pushLock);
		dequeSelector = (dequeSelector == 0) ? 1 : 0;
		draining = !dequeSelector;
	}

	// Delivered without holding pushLock, so a listener may publish; that goes to the other
	// queue and is delivered on the next flush. An event with no subscriber is dropped.
	auto& pending = queues[draining];
	while (!pending.empty())
	{
		ECXEvent event = std::move(pending.front());
		pending.pop_front();

		auto it = listeners.find(event.type);
		if (it == listeners.end())
			continue;
		for (auto listener : it->second)
		{
			listener->receive(event);
		}
	}
}

void ECXEventBroker::clear()
{
	std::scoped_lock<std::mutex> lock(pushLock);
	queues[0].clear();
	queues[1].clear();
}
