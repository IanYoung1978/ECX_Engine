#ifndef ECXEVENT_BROKER_H
#define ECXEVENT_BROKER_H
#include "ECXEventType.h"
#include "ECXEvent.h"
#include <deque>
#include <mutex>
#include <map>
#include <vector>

class IEventListener;
class ECXEvent;

class ECXEventBroker
{
public:
	ECXEventBroker();
	void Subscribe(IEventListener& listener, ECXEventType type);
	void publish(ECXEvent& Event);
	void flush();
	void clear();
private:
	int dequeSelector;
	std::deque<ECXEvent> queues[2];
	std::mutex pushLock;
	std::map<ECXEventType, std::vector<IEventListener*>> listeners;
};

#endif // !ECXEVENT_BROKER_H
