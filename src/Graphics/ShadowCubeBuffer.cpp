#include "ShadowCubeBuffer.h"
#include <GL\glew.h>


ShadowCubeBuffer::ShadowCubeBuffer()
{
	m_CubeMapTextures[0] = 0;
	m_CubeMapTextures[1] = 0;
	m_CubeMapTextures[2] = 0;
	m_CubeMapTextures[3] = 0;
	m_CubeMapTextures[4] = 0;
	m_CubeMapTextures[5] = 0;
}

bool ShadowCubeBuffer::init(int width, int height)
{
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	glGenFramebuffers(1, &m_FBOName);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOName);
	glGenTextures(1, &m_DepthTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_DepthTexture);
	m_Width = width;
	m_Height = height;
	for (int i = 0; i < 6; i++)
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


	}
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_DepthTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void ShadowCubeBuffer::bind()
{
	glViewport(0, 0, m_Width, m_Height);
	glBindFramebuffer(GL_FRAMEBUFFER, m_FBOName);
	glClear(GL_DEPTH_BUFFER_BIT);
}

unsigned int ShadowCubeBuffer::getDepthTexture()
{
	return m_DepthTexture;
}

void ShadowCubeBuffer::setLightType(LightType type)
{
	
}

LightType ShadowCubeBuffer::getLightType()
{
	return LightType::Point;
}


ShadowCubeBuffer::~ShadowCubeBuffer()
{
}
