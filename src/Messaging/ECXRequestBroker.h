#ifndef ECX_REQUEST_BROKER_H
#define ECX_REQUEST_BROKER_H
#include <map>

class ECXRequest;
class ECXResponse;
class IRequestResponder;
enum class ECXRequestType;

class ECXRequestBroker
{
public:
	void Subscribe(IRequestResponder& responder, ECXRequestType type);
	void publish(ECXRequest& Request, ECXResponse& Response);
private:
	std::map<ECXRequestType, IRequestResponder*> requestListeners;
};

#endif // !ECX_REQUEST_BROKER_H

