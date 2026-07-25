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
	if (!m_ExemptShadowBuffer.init(m_GBuffer.getDepthTexture(), m_FrameBuffer1.getBufferTexture()))
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
	// FrameBuffer::resize() deletes and recreates its texture (new handle); GBuffer::resize()
	// is currently a no-op so its depth texture handle is unchanged, but re-attaching both
	// here unconditionally is correct either way and avoids depending on that detail.
	m_ExemptShadowBuffer.resize(m_GBuffer.getDepthTexture(), m_FrameBuffer1.getBufferTexture());
	m_Width = width;
	m_Height = height;
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

void FrameBufferSet::ExemptShadowPass()
{
	m_ExemptShadowBuffer.bindForWriting(m_Width, m_Height);
}

void FrameBufferSet::EndExemptShadowPass()
{
	m_ExemptShadowBuffer.unbind();
}

void FrameBufferSet::FinalPass()
{
	// Blit colour to backbuffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_SwapBuffers ?
		m_FrameBuffer2.getBufferHandle() : m_FrameBuffer1.getBufferHandle());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, m_Width, m_Height,
		0, 0, m_Width, m_Height,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// Blit GBuffer depth to backbuffer for skybox depth testing
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GBuffer.getBufferHandle());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, m_Width, m_Height,
		0, 0, m_Width, m_Height,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

FrameBufferSet::~FrameBufferSet()
{
}
void FrameBufferSet::SkyboxPass()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, m_Width, m_Height);
}