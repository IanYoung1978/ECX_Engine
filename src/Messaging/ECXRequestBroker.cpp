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
	auto responder = requestListeners[Request.type];
	if (responder != nullptr)
	{
		Response = responder->receive(Request);
	}
	Response.response = ECXResponseType::Unsupported;
}
