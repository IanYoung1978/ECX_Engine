#include "ECXMessenger.h"

void ECXMessenger::Subscribe(ICommandListener& listener, ECXCommandType type)
{
	commands.Subscribe(listener, type);
}

void ECXMessenger::Subscribe(IEventListener& listener, ECXEventType type)
{
	events.Subscribe(listener, type);
}

void ECXMessenger::Subscribe(IEventListener& listener, std::vector<ECXEventType>& types)
{
	for (auto type : types)
	{
		events.Subscribe(listener, type);
	}
}

void ECXMessenger::Subscribe(IRequestResponder& responder, ECXRequestType type)
{
	requests.Subscribe(responder, type);
}

void ECXMessenger::publish(ECXCommand& Command)
{
	commands.publish(Command);
}

void ECXMessenger::publish(ECXEvent& Event)
{
	events.publish(Event);
}

void ECXMessenger::publish(ECXRequest& Request, ECXResponse& Response)
{
	requests.publish(Request, Response);
}

void ECXMessenger::flush()
{
	commands.flush();
	events.flush();
}

void ECXMessenger::purgeMessages()
{
	events.clear();
}
