#pragma once
#include "ECXCommand.h"

class ICommandListener
{
public:
	virtual void receive(ECXCommand& command) = 0;
	virtual ~ICommandListener() { ; }
};