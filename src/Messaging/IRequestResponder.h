#pragma once

#include "ECXRequest.h"
#include "ECXResponse.h"

class IRequestResponder
{
public:
	virtual ECXResponse& receive(ECXRequest& request) = 0;
	virtual ~IRequestResponder() { ; }
};