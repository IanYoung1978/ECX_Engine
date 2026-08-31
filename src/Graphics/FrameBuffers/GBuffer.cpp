#include "Graphics/FrameBuffers/GBuffer.h"

#include "Logging/ECX_Logging.h"

GBuffer::GBuffer()
{
	m_DepthBuffer = 0;
	m_Width = 0;
	m_Height = 0;
	m_Buffers.resize(TOTAL_BUFFERS, 0);
	m_BufferHandle = 0;
	m_DrawBuffers = new GLenum[TOTAL_BUFFERS];
	m_DrawBuffers[0] = GL_COLOR_ATTACHMENT0;
	m_DrawBuffers[1] = GL_COLOR_ATTACHMENT1;
	m_DrawBuffers[2] = GL_COLOR_ATTACHMENT2;
	m_DrawBuffers[3] = GL_COLOR_ATTACHMENT3;
	m_DrawBuffers[4] = GL_COLOR_ATTACHMENT4;
}

bool GBuffer::init(int width, int height)
{
    glGenFramebuffers(1, &m_BufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, m_BufferHandle);
    glGenTextures(TOTAL_BUFFERS, &m_Buffers[0]);

    for (int i = 0; i < TOTAL_BUFFERS; i++)
    {
        glBindTexture(GL_TEXTURE_2D, m_Buffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
            m_DrawBuffers[i],
            GL_TEXTURE_2D,
            m_Buffers[i],
            0);
    }

    glGenTextures(1, &m_DepthBuffer);
    glBindTexture(GL_TEXTURE_2D, m_DepthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthBuffer, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum Status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (Status != GL_FRAMEBUFFER_COMPLETE) {
        printf("FB error, status: 0x%x\n", Status);
        LOGGING::ECX_Logger::GetInstance()->LogMessage("framebuffer error ", LOGGING::LogLevel::CRITICAL);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_Width = width;
    m_Height = height;
    return true;
}

void GBuffer::resize(int width, int height)
{
}

void GBuffer::initFrame()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glBindFramebuffer(GL_FRAMEBUFFER, m_BufferHandle);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::setForWriting()
{
	glViewport(0, 0, m_Width, m_Height);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_BufferHandle);
	glDrawBuffers(TOTAL_BUFFERS, m_DrawBuffers);
}

void GBuffer::setForReading()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

unsigned int GBuffer::getGBufferTexture(FrameBufferType type)
{
	return m_Buffers[(size_t)type];
}


GBuffer::~GBuffer()
{
	glDeleteTextures(TOTAL_BUFFERS, &m_Buffers[0]);
	glDeleteFramebuffers(1, &m_BufferHandle);
	glDeleteRenderbuffers(1, &m_DepthBuffer);
}
