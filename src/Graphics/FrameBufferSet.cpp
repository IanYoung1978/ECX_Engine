#include "FrameBufferSet.h"



FrameBufferSet::FrameBufferSet()
{
	m_SwapBuffers = false;
	m_Height = 0;
	m_Width = 0;
}

bool FrameBufferSet::init(int width, int height)
{
	if (!m_GBuffer.init(width, height))
		return false;
	if (!m_FrameBuffer1.init(width, height))
		return false;
	if (!m_FrameBuffer2.init(width, height))
		return false;
	m_Width = width;
	m_Height = height;
	return true;
}

void FrameBufferSet::resize(int width, int height)
{
	m_GBuffer.resize(width, height);
	m_FrameBuffer1.resize(width, height);
	m_FrameBuffer2.resize(width, height);
}

void FrameBufferSet::initFrame()
{
	m_GBuffer.initFrame();
	m_SwapBuffers = false;
}

void FrameBufferSet::GeometryPass()
{
	m_GBuffer.setForWriting();
}

void FrameBufferSet::LightingPass(Shader& shader)
{
	m_FrameBuffer1.initFrame();
	m_GBuffer.setForReading();
	m_FrameBuffer1.setForWriting();
	shader.bindTexture(("positionMap"), 0, m_GBuffer.getGBufferTexture(FrameBufferType::Position));
	shader.bindTexture(("normalMap"), 1, m_GBuffer.getGBufferTexture(FrameBufferType::Normal));
	shader.bindTexture(("AlbedoMap"), 2, m_GBuffer.getGBufferTexture(FrameBufferType::Albedo));
	shader.bindTexture(("PBRMap"), 3, m_GBuffer.getGBufferTexture(FrameBufferType::PBR));
	shader.bindTexture(("glowMap"), 4, m_GBuffer.getGBufferTexture(FrameBufferType::Glow));
}

void FrameBufferSet::GlowPass(Shader& shader, bool first, bool last)
{
	if (first)
	{
		m_GBuffer.setForReading();
		shader.bindTexture(("glowMap"), 0, m_GBuffer.getGBufferTexture(FrameBufferType::Glow));
		m_FrameBuffer1.setForWriting();
		m_SwapBuffers = false;
	}
	else if (last)
	{
		if (m_SwapBuffers)
		{
			glCopyImageSubData(m_FrameBuffer2.getBufferTexture(), GL_TEXTURE_2D, 0, 0, 0, 0,
				m_GBuffer.getGBufferTexture(FrameBufferType::Glow), GL_TEXTURE_2D, 0, 0, 0, 0,
				m_Width, m_Height,1
			);
		}
		else
		{
			glCopyImageSubData(m_FrameBuffer1.getBufferTexture(), GL_TEXTURE_2D, 0, 0, 0, 0,
				m_GBuffer.getGBufferTexture(FrameBufferType::Glow), GL_TEXTURE_2D, 0, 0, 0, 0,
				m_Width, m_Height, 1
			);
		}
	}
	else
	{
		if (m_SwapBuffers)
		{
			m_FrameBuffer2.setForReading();
			m_FrameBuffer2.bindTextures();
			m_FrameBuffer1.setForWriting();
			m_SwapBuffers = false;
		}
		else
		{
			m_FrameBuffer1.setForReading();
			m_FrameBuffer1.bindTextures();
			m_FrameBuffer2.setForWriting();
			m_SwapBuffers = true;
		}
	}
}

void FrameBufferSet::PostProcessPass()
{
	if (m_SwapBuffers)
	{
		m_FrameBuffer2.bindTextures();
		m_FrameBuffer1.initFrame();
		m_FrameBuffer1.setForWriting();
		m_SwapBuffers = false;
	}
	else
	{
		m_FrameBuffer1.bindTextures();
		m_FrameBuffer2.initFrame();
		m_FrameBuffer2.setForWriting();
		m_SwapBuffers = true;
	}
}

void FrameBufferSet::UIPass()
{
	//TO:DO
}

void FrameBufferSet::FinalPass()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	if (m_SwapBuffers)
		m_FrameBuffer2.setForReading();
	else
		m_FrameBuffer1.setForReading();

	glBlitFramebuffer(0, 0, m_Width, m_Height,
		0, 0, m_Width, m_Height,
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}


FrameBufferSet::~FrameBufferSet()
{
}
