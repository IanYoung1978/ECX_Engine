#pragma once
#include "ECXEventType.h"
#include <any>

class ECXEvent
{
public:
	ECXEventType type;
	std::any args[8];
};

