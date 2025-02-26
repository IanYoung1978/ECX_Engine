#pragma once

#include "ECXEvent.h"

class IEventListener
{
public:
	virtual void receive(ECXEvent& Event) = 0;
	virtual ~IEventListener() { ; }
};