#pragma once
#include "Components/IComponent.h"
#include <glm\glm.hpp>
#include "LightData.h"
enum class LightType
{
	Point,
	Directional,
	SpotLight,
	Num_Lights
};


