#pragma once
#include "Graphics/Renderers/LightData.h"
class Shader;
class LightUniformBuffer
{
public:
	LightUniformBuffer();
	void init(Shader& shader);
	void updatePointLights(int numLights, const LightData* data);
	void updateSpotLights(int numLights, const SpotLightData* data);
	void bindPointLights();
	void bindSpotLights();
	~LightUniformBuffer();
private:
	static const int MAX_LIGHTS = 128;
	unsigned int m_PLBuffer;
	unsigned int m_SLBuffer;
	int m_PointLightBlockSize;
	int m_SpotLightBlockSize;
	unsigned int m_pointLightBlockIndex;
	unsigned int m_splBlockIndex;
	int m_MaxPoints;
	int m_MaxSpots;
};

