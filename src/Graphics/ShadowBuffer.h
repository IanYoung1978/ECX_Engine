#pragma once
#include "Light.h"
class ShadowBuffer
{
public:
	ShadowBuffer();
	virtual bool init(int width, int height);
	virtual void bind();
	virtual unsigned int getDepthTexture();
	void setLightType(LightType type);
	virtual LightType getLightType();
	virtual ~ShadowBuffer();
	int getWidth();
	int getHeight();
protected:
	unsigned int m_FBOName;
	unsigned int m_DepthTexture;
	int m_Width;
	int m_Height;
	LightType m_lightType;

};

