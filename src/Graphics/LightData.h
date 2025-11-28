#pragma once
#include <glm\glm.hpp>


// for working with GLSL data must be arranged in blocks of multiples of 16 bytes (equivelent to a vec4 float)
struct LightData
{								//base alignment		aligned offset		size
	glm::vec4 colour;			//16 bytes				0					16
	glm::vec4 position;			//16 bytes				16					32
	glm::vec4 attenuation;		//16 bytes				32					48
	float intensity;			//4	 bytes				36 (48 on gpu)		52
	glm::vec3 padding;			//12					48					64
}; //size = 52 bytes (64 padded)
struct DirLightData :
	LightData					//											64
{
	glm::vec4 addPadding;
	glm::vec4 direction;		//16										80											
}; //size = 96 bytes (16 bytes for ???)
struct SpotLightData :
	DirLightData
{
	float cutoffAngle;
	glm::vec3 Additionalpadding;
}; // size = 96 bytes