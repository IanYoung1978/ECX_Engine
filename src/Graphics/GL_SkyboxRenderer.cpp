#include "GL_SkyboxRenderer.h"
#include "Window/Window.h"
#include "Logging/ECX_Logging.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Entity/EC_DOD_EntityFactory.h"
#include "Components/EC_DOD_Components.h"
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

GL_SkyboxRenderer::~GL_SkyboxRenderer()
{
    if (m_CubeVAO) glDeleteVertexArrays(1, &m_CubeVAO);
    if (m_CubeVBO) glDeleteBuffers(1, &m_CubeVBO);
}

void GL_SkyboxRenderer::init(std::shared_ptr<Window> window)
{
    m_Window = window;

    if (!m_SkyboxShader.loadShader(
        "data/assets/shaders/skybox.vert",
        "data/assets/shaders/skybox.frag"))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load skybox shader",
            LOGGING::LogLevel::CRITICAL);
        return;
    }

    buildCubeVAO();
}

void GL_SkyboxRenderer::render(const glm::mat4& view, const glm::mat4& projection)
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    auto entities = manager.getEntitiesWithComponent(
        std::type_index(typeid(EC_DOD_Skybox))
    );

    if (entities.empty()) return;

    EntityID skyboxEntity = INVALID_ENTITY;
    for (EntityID e : entities) {
        if (manager.isAlive(e)) { skyboxEntity = e; break; }
    }
    if (skyboxEntity == INVALID_ENTITY) return;

    auto& skybox = manager.getComponent<EC_DOD_Skybox>(skyboxEntity);

    if (skybox.cubemapHandle == 0 && !skybox.hdrPath.empty())
        skybox.cubemapHandle = EC_DOD_EntityFactory::s_CubemapManager.getCubemap(skybox.hdrPath);

    if (skybox.cubemapHandle == 0) return;

    if (skybox.targetCubemapHandle == 0 && !skybox.targetHdrPath.empty())
        skybox.targetCubemapHandle = EC_DOD_EntityFactory::s_CubemapManager.getCubemap(skybox.targetHdrPath);

    glm::mat4 skyView = glm::mat4(glm::mat3(view));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    m_SkyboxShader.activate();
    m_SkyboxShader.setUniform("ViewTransform", skyView);
    m_SkyboxShader.setUniform("ProjTransform", projection);
    m_SkyboxShader.setUniform("blendFactor", skybox.blendFactor);

    m_SkyboxShader.bindCubemap("skybox", 0, skybox.cubemapHandle);

    bool hasTarget = skybox.targetCubemapHandle != 0;
    m_SkyboxShader.setUniform("hasTarget", hasTarget ? 1 : 0);
    if (hasTarget)
        m_SkyboxShader.bindCubemap("targetSkybox", 1, skybox.targetCubemapHandle);

    glBindVertexArray(m_CubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glUseProgram(0);
}

void GL_SkyboxRenderer::buildCubeVAO()
{
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