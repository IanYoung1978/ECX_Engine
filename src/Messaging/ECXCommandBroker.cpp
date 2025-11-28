#include "ECXCommandBroker.h"
#include "ECXCommandType.h"
#include "ECXCommand.h"
#include "ICommandListener.h"

ECXCommandBroker::ECXCommandBroker()
{
	dequeSelector = 0;
}

void ECXCommandBroker::Subscribe(ICommandListener& listener, ECXCommandType type)
{
	listeners[type].push_back(&listener);
}

void ECXCommandBroker::publish(ECXCommand& Command)
{
	std::scoped_lock<std::mutex> lock(pushLock);
	queues[dequeSelector].push_back(Command);
}

void ECXCommandBroker::flush()
{
	while (!queues[!dequeSelector].empty())
	{
		auto& listenerSet = listeners[queues[!dequeSelector].front().type];
		if (listenerSet.size() > 0)
		{
			auto Command = queues[!dequeSelector].front();
			queues[!dequeSelector].pop_front();
			for (auto listener : listenerSet)
			{
				listener->receive(Command);
			}
		}

	}
	dequeSelector = (dequeSelector == 0) ? 1 : 0;
}
