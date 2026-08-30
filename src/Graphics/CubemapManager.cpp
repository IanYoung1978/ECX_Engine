#include "CubemapManager.h"
#include "Shader.h"
#include "Logging/ECX_Logging.h"
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

CubemapManager::~CubemapManager()
{
    for (auto& pair : m_Cubemaps)
        glDeleteTextures(1, &pair.second);
    for (auto& pending : m_Pending)
        if (pending.pixels) stbi_image_free(pending.pixels);
    if (m_CubeVAO) glDeleteVertexArrays(1, &m_CubeVAO);
    if (m_CubeVBO) glDeleteBuffers(1, &m_CubeVBO);
}

void CubemapManager::loadHDR(const std::string& filename)
{
    // Already loaded or pending
    if (m_Cubemaps.count(filename)) return;
    for (auto& p : m_Pending)
        if (p.filename == filename) return;

    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    float* data = stbi_loadf(filename.c_str(), &width, &height, &channels, 0);

    if (!data) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load HDR: " + filename,
            LOGGING::LogLevel::SEVERE);
        return;
    }

    m_Pending.push_back({ filename, data, width, height });

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "HDR queued for finalization: " + filename +
        " (" + std::to_string(width) + "x" + std::to_string(height) + ")",
        LOGGING::LogLevel::INFORMATION);
}

void CubemapManager::finalizeAll()
{
    for (auto& pending : m_Pending) {
        unsigned int cubemap = convertEquirectToCubemap(
            pending.pixels, pending.width, pending.height);
        stbi_image_free(pending.pixels);
        pending.pixels = nullptr;

        if (cubemap != 0) {
            m_Cubemaps[pending.filename] = cubemap;
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "Cubemap finalized: " + pending.filename,
                LOGGING::LogLevel::INFORMATION);
        }
    }
    m_Pending.clear();
}

unsigned int CubemapManager::getCubemap(const std::string& filename)
{
    auto it = m_Cubemaps.find(filename);
    if (it != m_Cubemaps.end()) return it->second;
    return 0;
}

void CubemapManager::buildCubeVAO()
{
    if (m_CubeVAO) return;

    float vertices[] = {
        -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_CubeVAO);
    glGenBuffers(1, &m_CubeVBO);
    glBindVertexArray(m_CubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

unsigned int CubemapManager::convertEquirectToCubemap(float* pixels, int width, int height)
{
    // Upload equirect as 2D texture
    unsigned int equirectTex = 0;
    glGenTextures(1, &equirectTex);
    glBindTexture(GL_TEXTURE_2D, equirectTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum err = glGetError();
    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Equirect texture upload: handle=" + std::to_string(equirectTex) +
        " size=" + std::to_string(width) + "x" + std::to_string(height) +
        " GL error=" + std::to_string(err),
        LOGGING::LogLevel::INFORMATION
    );

    // Create cubemap
    unsigned int cubemap = 0;
    glGenTextures(1, &cubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    for (int i = 0; i < 6; i++)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // FBO for rendering each face
    unsigned int fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "CubemapManager: equirect-to-cubemap FBO incomplete, status=" + std::to_string(fboStatus),
            LOGGING::LogLevel::CRITICAL
        );
    }
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };

    Shader conversionShader;
    if (!conversionShader.loadShader(
        "data/assets/shaders/equirect_to_cubemap.vert",
        "data/assets/shaders/equirect_to_cubemap.frag"))
    {
        glDeleteTextures(1, &cubemap);
        glDeleteTextures(1, &equirectTex);
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &rbo);
        return 0;
    }

    buildCubeVAO();

    conversionShader.activate();
    conversionShader.bindTexture("equirectangularMap", 0, equirectTex);
    conversionShader.setUniform("ProjTransform", captureProjection);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    for (int i = 0; i < 6; i++) {
        conversionShader.setUniform("ViewTransform", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(m_CubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        GLenum faceErr = glGetError();
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Face " + std::to_string(i) + " rendered, GL error=" + std::to_string(faceErr),
            LOGGING::LogLevel::INFORMATION
        );
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &equirectTex);

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Cubemap conversion complete, handle=" + std::to_string(cubemap),
        LOGGING::LogLevel::INFORMATION
    );

    return cubemap;
}