#pragma once
#include "ShadowBuffer.h"
class ShadowCubeBuffer :
	public ShadowBuffer
{
public:
	ShadowCubeBuffer();
	virtual bool init(int width, int height);
	virtual void bind();
	virtual unsigned int getDepthTexture();
	virtual void setLightType(LightType type);
	virtual LightType getLightType();
	virtual ~ShadowCubeBuffer();
private:
	unsigned int m_CubeMapTextures[6];
};

