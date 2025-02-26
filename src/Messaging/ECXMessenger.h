#pragma once
#include <deque>
#include <map>
#include <vector>

#include "ECXCommand.h"
#include "ECXRequest.h"
#include "ECXResponse.h"
#include "ECXEvent.h"
#include "ICommandListener.h"
#include "IEventListener.h"
#include "IRequestResponder.h"
#include "ECXEventBroker.h"
#include "ECXCommandBroker.h"
#include "ECXRequestBroker.h"

class ECXMessenger
{
public:
	ECXMessenger() = default;
	~ECXMessenger() = default;
	ECXMessenger(ECXMessenger& messenger) = delete;
	ECXMessenger(ECXMessenger&& messenger) = delete;
	ECXMessenger operator =(ECXMessenger& messenger) = delete;

	void Subscribe(ICommandListener& listener, ECXCommandType type);
	void Subscribe(IEventListener& listener, ECXEventType type);
	void Subscribe(IEventListener& listener, std::vector<ECXEventType>& types);
	void Subscribe(IRequestResponder& responder, ECXRequestType type);

	void publish(ECXCommand& Command);
	void publish(ECXEvent& Event);
	void publish(ECXRequest& Request, ECXResponse& Response);

	void flush();

	void purgeMessages();
private:
	ECXEventBroker events;
	ECXCommandBroker commands;
	ECXRequestBroker requests;
};