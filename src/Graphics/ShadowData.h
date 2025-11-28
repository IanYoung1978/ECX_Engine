#pragma once
#include <glm\glm.hpp>
struct ShadowData
{
	glm::mat4 X_Pos;
	glm::mat4 X_Neg;
	glm::mat4 Y_Pos;
	glm::mat4 Y_Neg;
	glm::mat4 Z_Pos;
	glm::mat4 Z_Neg;
};
