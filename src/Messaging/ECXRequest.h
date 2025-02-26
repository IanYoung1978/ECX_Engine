#pragma once
#include "ECXRequestType.h"
#include <any>

class ECXRequest
{
public:
	ECXRequestType type;
	std::any args[8];
};
