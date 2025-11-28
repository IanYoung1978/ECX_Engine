#pragma once
#include <vector>
#include <any>
#include "ECXResponseType.h"

class ECXResponse
{
public:
	ECXResponseType response;
	std::vector<std::any> responseData;
};