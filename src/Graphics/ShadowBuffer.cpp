#include "ShadowBuffer.h"
#include <GL\glew.h>


ShadowBuffer::ShadowBuffer()
{
	m_FBOName = 0;
	m_DepthTexture = 0;
	m_Width = 0;
	m_Height = 0;
	m_lightType = LightType::Directional;
}

bool ShadowBuffer::init(int width, int height)
{
	m_Width = width;
	m_Height = height;
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	glGenFramebuffers(1, &m_FBOName);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOName);

	// Depth texture. Slower than a depth buffer, but you can sample it later in your shader
	glGenTextures(1, &m_DepthTexture);
	glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

						   // Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

void ShadowBuffer::bind()
{
	glViewport(0, 0, m_Width, m_Height);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOName);
	glClear(GL_DEPTH_BUFFER_BIT);
}

unsigned int ShadowBuffer::getDepthTexture()
{
	return m_DepthTexture;
}

void ShadowBuffer::setLightType(LightType type)
{
	m_lightType = type;
}

LightType ShadowBuffer::getLightType()
{
	return m_lightType;
}


ShadowBuffer::~ShadowBuffer()
{
	if(m_DepthTexture)
		glDeleteTextures(1, &m_DepthTexture);
	glDeleteFramebuffers(1, &m_FBOName);
}

int ShadowBuffer::getWidth()
{
	return m_Width;
}

int ShadowBuffer::getHeight()
{
	return m_Height;
}
