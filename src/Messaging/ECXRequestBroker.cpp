#include "ECXRequestBroker.h"
#include "ECXRequest.h"
#include "ECXResponse.h"
#include "ECXRequestType.h"
#include "IRequestResponder.h"

void ECXRequestBroker::Subscribe(IRequestResponder& responder, ECXRequestType type)
{
	requestListeners[type] = &responder;
}

void ECXRequestBroker::publish(ECXRequest& Request, ECXResponse& Response)
{
	auto it = requestListeners.find(Request.type);
	if (it != requestListeners.end() && it->second != nullptr)
	{
		Response = it->second->receive(Request);
	}
	else
	{
		Response.response = ECXResponseType::Unsupported;
	}
}
