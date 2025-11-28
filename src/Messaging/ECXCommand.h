#pragma once
#include <any>
#include "ECXCommandType.h"

class ECXCommand
{
public:
	std::any args[2];
	ECXCommandType type;
};