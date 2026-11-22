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
    // Swapped under pushLock so a publisher can never push into the queue being drained below.
    int draining;
    {
        std::scoped_lock<std::mutex> lock(pushLock);
        dequeSelector = (dequeSelector == 0) ? 1 : 0;
        draining = !dequeSelector;
    }

    // Delivered without holding pushLock, so a listener may publish; that is delivered on the
    // next flush. A command with no subscriber is dropped.
    auto& pending = queues[draining];
    while (!pending.empty())
    {
        ECXCommand command = std::move(pending.front());
        pending.pop_front();

        auto it = listeners.find(command.type);
        if (it == listeners.end())
            continue;
        for (auto listener : it->second)
        {
            listener->receive(command);
        }
    }
}
