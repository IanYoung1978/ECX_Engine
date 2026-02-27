#include "GL_Deferred_Renderer.h"
#include "Components/EC_DOD_Components.h"
#include "BufferType.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Entity/EC_DOD_EntityFactory.h"
#include "Logging/ECX_Logging.h"
#include "Window/Window.h"
#include "Graphics/ObjModel.h"
#include "Graphics/TextureSet.h"

GL_Deferred_Renderer::GL_Deferred_Renderer()
    : m_MSAA(false)
    , m_DOF(false)
    , m_HDR(false)
    , m_FS_QuadHandle(0)
    , m_FS_QuadIndices(0)
    , m_FS_QuadTex(0)
    , m_FS_QuadVerts(0)
{
}

GL_Deferred_Renderer::~GL_Deferred_Renderer() {
}

void GL_Deferred_Renderer::init(std::shared_ptr<Window> window) {
    m_Window = window;
    m_DebugRenderer.init(window);
    glm::vec3 verts[] = {
        glm::vec3(-1.0f,-1.0f,0.0f),
        glm::vec3(1.0f,-1.0f,0.0f),
        glm::vec3(1.0f, 1.0f,0.0f),
        glm::vec3(-1.0f, 1.0f,0.0f)
    };

    glm::vec2 texCoords[] = {
        glm::vec2(0.0f,0.0f),
        glm::vec2(1.0f,0.0f),
        glm::vec2(1.0f,1.0f),
        glm::vec2(0.0f,1.0f)
    };
    //////////////////////////////////////////////////////////////////////
//				uv(0,0)								uv(1,0)										
//					-------------------------------------
//			v(-1,1)	| i(3)						   i(2) |	v(1,1)
//					|									|
//					|									|
//					|									|
//			v(-1,-1)| i(0)						   i(1) |	v(1,-1)
//					-------------------------------------
//				uv(0,1)								uv(1,1)
///////////////////////////////////////////////////////////////////////
    unsigned int indices[] = { 0,1,2,0,2,3 };

    glGenVertexArrays(1, &m_FS_QuadHandle);
    glBindVertexArray(m_FS_QuadHandle);
    glGenBuffers(1, &m_FS_QuadVerts);
    glBindBuffer(GL_ARRAY_BUFFER, m_FS_QuadVerts);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 4, verts, GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)BufferType::Vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray((GLuint)BufferType::Vertex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &m_FS_QuadTex);
    glBindBuffer(GL_ARRAY_BUFFER, m_FS_QuadTex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 4, texCoords, GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)BufferType::TextureCoordinate, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray((GLuint)BufferType::TextureCoordinate);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glGenBuffers(1, &m_FS_QuadIndices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_FS_QuadIndices);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_LightPassShader = std::make_shared<Shader>();

    if (!m_LightPassShader->loadShader(std::string("data/assets/shaders/lightpass.vert"), std::string("data/assets/shaders/lightpass.frag"))) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load light pass shader", LOGGING::LogLevel::CRITICAL);
    }

    if (!m_ShadowShader.loadShader(std::string("data/assets/shaders/shadow.vert"), std::string("data/assets/shaders/shadow.frag"))) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load point shadow pass shader", LOGGING::LogLevel::CRITICAL);
    }

    if (!m_ShadowDirLightShader.loadShader(std::string("data/assets/shaders/ShadowLightPass.vert"), std::string("data/assets/shaders/DirLightShadowPBR.frag"))) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load directional shadow pass shader", LOGGING::LogLevel::CRITICAL);
    }

    if (!m_ShadowSpotLightShader.loadShader(std::string("data/assets/shaders/ShadowLightPass.vert"), std::string("data/assets/shaders/SpotlightShadowPBR.frag"))) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load spot shadow pass shader", LOGGING::LogLevel::CRITICAL);
    }

    if (!m_BloomHShader.loadShader(std::string("data/assets/shaders/final.vert"), std::string("data/assets/shaders/bloomH.frag")) ||
        !m_BloomVShader.loadShader(std::string("data/assets/shaders/final.vert"), std::string("data/assets/shaders/bloomV.frag"))) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load bloom shader", LOGGING::LogLevel::CRITICAL);
    }

    m_FrameBuffer.init(window->getWidth(), window->getHeight());
    m_LightBuffer.init((*m_LightPassShader));
    m_ShadowBuffer.init(2048, 2048);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GL_Deferred_Renderer::renderScene() {
    m_FrameBuffer.initFrame();
    geometryPass();
	glowPass();
    lightPass();
    finalPass();
    debugPass();
}

void GL_Deferred_Renderer::changeResolution(int width, int height) {
    m_FrameBuffer.resize(width, height);
}

void GL_Deferred_Renderer::geometryPass() {
    auto& manager = EC_DOD_EntityManager::getInstance();

    auto cameras = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Spatial)),
        std::type_index(typeid(EC_DOD_Camera))
        });

    for (EntityID cameraID : cameras) {
        if (!manager.isAlive(cameraID)) continue;

        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(cameraID);
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);

        if (!camera.isActive) continue;
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov),
            (float)m_Window->getWidth() / m_Window->getHeight(),
            camera.nearPlane,
            camera.farPlane
        );

        glm::mat4 view = camera.viewMatrix;

        m_FrameBuffer.GeometryPass();

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        auto renderables = manager.getEntitiesWithComponents({
            std::type_index(typeid(EC_DOD_Transform)),
            std::type_index(typeid(EC_DOD_GraphicsData))
            });

        for (EntityID entityID : renderables) {
            if (!manager.isAlive(entityID)) continue;

            const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
            auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);

            if (!gfx.visible || !gfx.model || !gfx.shader) continue;

            gfx.shader->activate();

            gfx.shader->setUniform("camPos", spatial.position);
            gfx.shader->setUniform("ViewTransform", view);
            gfx.shader->setUniform("ProjTransform", projection);
            gfx.shader->setUniform("ModelTransform", transform.matrix);

            if (gfx.hasTextures && gfx.textureSet) {
                gfx.shader->setUniform("hasMaterial", 1);
                gfx.textureSet->bindTextures(gfx.shader);
            }
            else {
                gfx.shader->setUniform("hasMaterial", 0);
                gfx.shader->setUniform("incolour", gfx.colour);
            }

            glBindVertexArray(gfx.getMeshHandle());
            glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Unbind textures
            for (int i = 0; i < 10; i++) {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        break;
    }
}

void GL_Deferred_Renderer::lightPass() {
    if (!m_LightPassShader) return;

    auto& manager = EC_DOD_EntityManager::getInstance();

    auto cameras = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Spatial)),
        std::type_index(typeid(EC_DOD_Camera))
        });

    for (EntityID cameraID : cameras) {
        if (!manager.isAlive(cameraID)) continue;

        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(cameraID);
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);

        if (!camera.isActive) continue;

        updateLights();

        m_LightPassShader->activate();
        m_FrameBuffer.LightingPass(*m_LightPassShader);
        m_LightBuffer.updatePointLights((int)m_Points.size(), m_Points.data());
        m_LightBuffer.updateSpotLights((int)m_Spots.size(), m_Spots.data());

        m_LightBuffer.bindPointLights();
        m_LightPassShader->setUniform("NumPoints", (int)m_Points.size());
        m_LightBuffer.bindSpotLights();
        m_LightPassShader->setUniform("NumSpots", (int)m_Spots.size());
        m_LightPassShader->setUniform("WSCamPos", spatial.position);

        if (!m_Directionals.empty()) {
            m_LightPassShader->setLight("dirLight", m_Directionals[0]);
        }

        renderQuad();
        shadowLightingPass();
        break;
    }
}

void GL_Deferred_Renderer::updateLights() {
    m_Directionals.clear();
    m_Spots.clear();
    m_Points.clear();
    m_ShadowDirs.clear();
    m_ShadowSpots.clear();
    m_ShadowPoints.clear();

    auto& manager = EC_DOD_EntityManager::getInstance();
    auto* lightArray = manager.getComponentArray<EC_DOD_Light>();

    if (!lightArray) return;

    std::shared_lock lock(lightArray->getMutex());
    auto& lights = lightArray->getData();

    for (size_t i = 0; i < lights.size(); i++) {
        EntityID entityID = lightArray->getEntity(i);
        if (!manager.isAlive(entityID)) continue;

        const auto& light = lights[i];

        if (light.type == EC_DOD_Light::Type::Directional) {
            DirLightData data;
            data.direction = glm::vec4(light.direction, 0.0f);
            data.colour = glm::vec4(light.colour, 1.0f);
            data.intensity = light.intensity;

            if (!light.castsShadow) {
                m_Directionals.push_back(data);
			}
            else {
                m_ShadowDirs.push_back(data);
            }
        }
        else if (light.type == EC_DOD_Light::Type::Spot) {
            SpotLightData data;
            data.position = glm::vec4(light.position, 1.0f);
            data.direction = glm::vec4(light.direction, 0.0f);
            data.colour = glm::vec4(light.colour, 1.0f);
            data.intensity = light.intensity;
            data.cutoffAngle = light.cutoffAngle;
            data.attenuation = glm::vec4(light.attenuation, 0.0f);

            if (!light.castsShadow) {
                m_Spots.push_back(data);
            }
            else {
                m_ShadowSpots.push_back(data);
            }
        }
        else {
            LightData data;
            data.position = glm::vec4(light.position, 1.0f);
            data.colour = glm::vec4(light.colour, 1.0f);
            data.intensity = light.intensity;
            data.attenuation = glm::vec4(light.attenuation, 0.0f);

            if (!light.castsShadow) {
                m_Points.push_back(data);
            }
            
            else {
                m_ShadowPoints.push_back(data);
            }
        }
    }
}

void GL_Deferred_Renderer::shadowDirPass(ShadowBuffer& target, DirLightData& light) {
    glm::mat4 biasMatrix(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f
    );

    glm::vec3 eye = glm::vec3(-light.direction);
    glm::vec3 up;
    up[0] = eye[1] - eye[2];
    up[1] = eye[2] - eye[0];
    up[2] = eye[0] - eye[1];

    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), up);
    glm::mat4 projection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -10.0f, 20.0f);
    m_ShadowDirMatrix = biasMatrix * projection * view;

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0, 2.0);
    glCullFace(GL_FRONT);
    target.bind();
    glClearDepth(1.0);

    auto& manager = EC_DOD_EntityManager::getInstance();
    auto renderables = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Transform)),
        std::type_index(typeid(EC_DOD_GraphicsData))
        });

    for (EntityID entityID : renderables) {
        if (!manager.isAlive(entityID)) continue;

        const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
        const auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);

        if (gfx.getMeshHandle() == 0) continue;

        m_ShadowShader.activate();
        m_ShadowShader.setUniform("ViewTransform", view);
        m_ShadowShader.setUniform("ProjTransform", projection);
        m_ShadowShader.setUniform("ModelTransform", transform.matrix);

        glBindVertexArray(gfx.getMeshHandle());
        glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
}

void GL_Deferred_Renderer::shadowSpotPass(ShadowBuffer& target, SpotLightData& light) {
    glm::mat4 biasMatrix(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f
    );

    glm::vec3 position = light.position;
    glm::vec3 direction = light.direction;

    glm::vec3 up;
    up[0] = direction[1] - direction[2];
    up[1] = direction[2] - direction[0];
    up[2] = direction[0] - direction[1];

    glm::mat4 view = glm::lookAt(position, position + direction, up);
    glm::mat4 projection = glm::perspective(45.0f, 1.0f, 1.0f, 100.0f);
    m_ShadowSpotMatrix = biasMatrix * projection * view;

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0, 2.0);
    glCullFace(GL_FRONT);
    target.bind();
    glClearDepth(1.0);

    auto& manager = EC_DOD_EntityManager::getInstance();
    auto renderables = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Transform)),
        std::type_index(typeid(EC_DOD_GraphicsData))
        });

    for (EntityID entityID : renderables) {
        if (!manager.isAlive(entityID)) continue;

        const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
        const auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);

        if (gfx.getMeshHandle() == 0) continue;

        m_ShadowShader.activate();
        m_ShadowShader.setUniform("ViewTransform", view);
        m_ShadowShader.setUniform("ProjTransform", projection);
        m_ShadowShader.setUniform("ModelTransform", transform.matrix);

        glBindVertexArray(gfx.getMeshHandle());
        glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
}

void GL_Deferred_Renderer::shadowPointPass(ShadowBuffer& target, LightData& light) {
}

void GL_Deferred_Renderer::shadowLightingPass() {
    auto& manager = EC_DOD_EntityManager::getInstance();

    auto cameras = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Spatial)),
        std::type_index(typeid(EC_DOD_Camera))
        });

    for (EntityID cameraID : cameras) {
        if (!manager.isAlive(cameraID)) continue;

        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(cameraID);

        for (size_t i = 0; i < m_ShadowDirs.size(); i++) {
            shadowDirPass(m_ShadowBuffer, m_ShadowDirs[i]);

            m_ShadowDirLightShader.activate();
            m_FrameBuffer.LightingPass(m_ShadowDirLightShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ShadowDirLightShader.setUniform("WSCamPos", spatial.position);
            m_ShadowDirLightShader.setDirLight("dirLight", m_ShadowDirs[i]);
            m_ShadowDirLightShader.setUniform("ShadowTransform", m_ShadowDirMatrix);
            m_ShadowDirLightShader.bindTexture("shadowMap", 5, m_ShadowBuffer.getDepthTexture());
            renderQuad();
            glDisable(GL_BLEND);
        }

        for (size_t i = 0; i < m_ShadowSpots.size(); i++) {
            shadowSpotPass(m_ShadowBuffer, m_ShadowSpots[i]);

            m_ShadowSpotLightShader.activate();
            m_FrameBuffer.LightingPass(m_ShadowSpotLightShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ShadowSpotLightShader.setUniform("WSCamPos", spatial.position);
            m_ShadowSpotLightShader.setSpotLight("spotLight", m_ShadowSpots[i]);
            m_ShadowSpotLightShader.setUniform("ShadowTransform", m_ShadowSpotMatrix);
            m_ShadowSpotLightShader.bindTexture("shadowMap", 5, m_ShadowBuffer.getDepthTexture());
            renderQuad();
            glDisable(GL_BLEND);
        }

        for (size_t i = 0; i < m_ShadowPoints.size(); i++) {
            shadowPointPass(m_ShadowBuffer, m_ShadowPoints[i]);

            m_ShadowPointLightShader.activate();
            m_FrameBuffer.LightingPass(m_ShadowPointLightShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ShadowPointLightShader.setUniform("WSCamPos", spatial.position);
            m_ShadowPointLightShader.setLight("pointLight", m_ShadowPoints[i]);
            m_ShadowPointLightShader.setUniform("ShadowTransform", m_ShadowDirMatrix);
            m_ShadowPointLightShader.bindTexture("shadowMap", 5, m_ShadowBuffer.getDepthTexture());
            renderQuad();
            glDisable(GL_BLEND);
        }

        break;
    }

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GL_Deferred_Renderer::postProcess() {
    if (!m_PostProcessShaders.empty()) {
        if (m_MSAA) {
            m_PostProcessShaders[(size_t)PostProcess::MSAA]->activate();
            m_FrameBuffer.PostProcessPass();
            renderQuad();
        }
        if (m_HDR) {
            m_PostProcessShaders[(size_t)PostProcess::HDR]->activate();
            m_FrameBuffer.PostProcessPass();
            renderQuad();
        }
        if (m_DOF) {
            m_PostProcessShaders[(size_t)PostProcess::DOF]->activate();
            m_FrameBuffer.PostProcessPass();
            renderQuad();
        }
    }
}

void GL_Deferred_Renderer::glowPass() {
    m_BloomHShader.activate();
    m_FrameBuffer.GlowPass(m_BloomHShader, true, false);
    renderQuad();

    for (int i = 0; i < 5; i++) {
        m_BloomHShader.activate();
        m_FrameBuffer.GlowPass(m_BloomHShader, false, false);
        renderQuad();
        m_BloomVShader.activate();
        m_FrameBuffer.GlowPass(m_BloomVShader, false, false);
        renderQuad();
    }

    m_FrameBuffer.GlowPass(m_BloomHShader, false, true);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GL_Deferred_Renderer::finalPass() {
    m_FrameBuffer.FinalPass();
}

void GL_Deferred_Renderer::renderQuad() {
    glBindVertexArray(m_FS_QuadHandle);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GL_Deferred_Renderer::debugPass()
{
    if (!m_DebugRenderer.isEnabled()) return;

    auto& manager = EC_DOD_EntityManager::getInstance();

    auto cameras = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Spatial)),
        std::type_index(typeid(EC_DOD_Camera))
        });

    for (EntityID cameraID : cameras) {
        if (!manager.isAlive(cameraID)) continue;

        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);
        if (!camera.isActive) continue;

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov),
            (float)m_Window->getWidth() / m_Window->getHeight(),
            camera.nearPlane,
            camera.farPlane
        );

        m_DebugRenderer.render(camera.viewMatrix, projection);
        break;
    }
}