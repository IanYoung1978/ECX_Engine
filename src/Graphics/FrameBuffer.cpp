#include "FrameBuffer.h"
#include <GL\glew.h>
#include "Logging/ECX_Logging.h"

FrameBuffer::FrameBuffer()
{
	m_BufferHandle = 0;
	m_Height = 0;
	m_Width = 0;
	m_RenderTexture = 0;
}

bool FrameBuffer::init(int width, int height)
{
	//crate fbo
	glGenFramebuffers(1, &m_BufferHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, m_BufferHandle);

	glGenTextures(1, &m_RenderTexture);
	glBindTexture(GL_TEXTURE_2D, m_RenderTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RenderTexture, 0);

	GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (Status != GL_FRAMEBUFFER_COMPLETE) {
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Framebuffer creation error ", LOGGING::LogLevel::CRITICAL);
		return false;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_Width = width;
	m_Height = height;
	return true;
}

void FrameBuffer::resize(int width, int height)
{
	glDeleteBuffers(1, &m_RenderTexture);
	glDeleteFramebuffers(1, &m_BufferHandle);
	init(width, height);
}

void FrameBuffer::initFrame()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_BufferHandle);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	//glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FrameBuffer::setForWriting()
{
	glViewport(0, 0, m_Width, m_Height);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_BufferHandle);
	GLenum att = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &att);
}

void FrameBuffer::setForReading()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_BufferHandle);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
}

void FrameBuffer::bindTextures()
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_RenderTexture);
}

unsigned int FrameBuffer::getBufferTexture()
{
	return m_RenderTexture;
}

FrameBuffer::~FrameBuffer()
{
	glDeleteBuffers(1,&m_RenderTexture);
	glDeleteFramebuffers(1, &m_BufferHandle);
}
