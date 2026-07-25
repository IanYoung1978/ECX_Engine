#include "ExemptShadowCompositeBuffer.h"
#include <GL\glew.h>
#include "Logging/ECX_Logging.h"

ExemptShadowCompositeBuffer::ExemptShadowCompositeBuffer()
{
}

ExemptShadowCompositeBuffer::~ExemptShadowCompositeBuffer()
{
    if (m_FBO)
        glDeleteFramebuffers(1, &m_FBO);
}

bool ExemptShadowCompositeBuffer::init(unsigned int gbufferDepthTexture, unsigned int colourTexture)
{
    glGenFramebuffers(1, &m_FBO);
    attach(gbufferDepthTexture, colourTexture);

    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    if (!ok)
        LOGGING::ECX_Logger::GetInstance()->LogMessage("ExemptShadowCompositeBuffer framebuffer incomplete", LOGGING::LogLevel::CRITICAL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}

void ExemptShadowCompositeBuffer::resize(unsigned int gbufferDepthTexture, unsigned int colourTexture)
{
    attach(gbufferDepthTexture, colourTexture);
}

void ExemptShadowCompositeBuffer::attach(unsigned int gbufferDepthTexture, unsigned int colourTexture)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gbufferDepthTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colourTexture, 0);
    GLenum drawBuf = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ExemptShadowCompositeBuffer::bindForWriting(int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    glDepthMask(GL_FALSE);
}

void ExemptShadowCompositeBuffer::unbind()
{
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
