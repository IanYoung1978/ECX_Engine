#include "FrameBufferSet.h"



FrameBufferSet::FrameBufferSet()
{
	m_SwapBuffers = false;
	m_Height = 0;
	m_Width = 0;
}

bool FrameBufferSet::init(int width, int height, int bloomMipLevels)
{
	if (!m_GBuffer.init(width, height))
		return false;
	if (!m_FrameBuffer1.init(width, height))
		return false;
	if (!m_FrameBuffer2.init(width, height))
		return false;
	if (!m_ExemptShadowBuffer.init(m_GBuffer.getDepthTexture(), m_FrameBuffer1.getBufferTexture()))
		return false;
	if (!m_BloomChain.init(width, height, bloomMipLevels))
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
	m_BloomChain.resize(width, height);
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

void FrameBufferSet::BeginBloomChain()
{
	m_GBuffer.setForReading();
}

unsigned int FrameBufferSet::getBloomDownsampleSource(int level)
{
	return level == 0
		? m_GBuffer.getGBufferTexture(FrameBufferType::Glow)
		: m_BloomChain.getLevelTexture(level - 1);
}

void FrameBufferSet::EndBloomChain()
{
	// Level 0 is half the G-buffer's resolution, so this must be a scaled blit (not a 1:1
	// glCopyImageSubData, which would only fill the destination's bottom-left quadrant and
	// leave the rest of the Glow texture stale).
	glm::ivec2 size = m_BloomChain.getLevelSize(0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_BloomChain.getLevelFBO(0));
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GBuffer.getBufferHandle());
	glDrawBuffer(GL_COLOR_ATTACHMENT4); // Glow, per FrameBufferType order (Position,Normal,Albedo,PBR,Glow)
	glBlitFramebuffer(0, 0, size.x, size.y,
		0, 0, m_Width, m_Height,
		GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
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