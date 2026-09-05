#include "Graphics/Renderers/GL_Deferred_Renderer.h"
#include "Components/EC_DOD_Components.h"
#include "Components/EC_CollisionLayers.h"
#include "Graphics/FrameBuffers/BufferType.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Entity/EC_DOD_EntityFactory.h"
#include "Logging/ECX_Logging.h"
#include "Window/Window.h"
#include "Graphics/Models/ObjModel.h"
#include "Graphics/Textures/TextureSet.h"
#include <chrono>
#include "Messaging/ECXMessenger.h"
#include "Messaging/ECXRequest.h"
#include "Messaging/ECXResponse.h"
#include "Spatial/RayQueryHit.h"
#include "SceneManager/EC_GameScene.h"
#include "UI/EC_UI_Components.h"
#include "Graphics/Renderers/DebugVisualization.h"
#include <algorithm>
#include <typeindex>
#include <array>
#include <limits>

namespace {
    // Not tunable: a cubemap's 6 faces exactly tile a full sphere only when
    // each face covers precisely 90 degrees, so a point light's omnidirectional
    // shadow cubemap requires this FOV geometrically, unlike the spot/point
    // near/far planes below which are genuine render-quality tradeoffs.
    constexpr float kCubemapFaceFovDegrees = 90.0f;

    // The 8 corners of the camera's view frustum in world space, clamped to [nearPlane,
    // farPlane] rather than the camera's own draw distance - used to fit the directional
    // shadow's ortho box to what the camera can actually see (see shadowDirPass).
    std::array<glm::vec3, 8> computeCameraFrustumCorners(
        const glm::mat4& camView, float fovDeg, float aspect, float nearPlane, float farPlane)
    {
        glm::mat4 proj = glm::perspective(glm::radians(fovDeg), aspect, nearPlane, farPlane);
        glm::mat4 invViewProj = glm::inverse(proj * camView);
        std::array<glm::vec3, 8> corners;
        int idx = 0;
        for (int x = 0; x < 2; x++)
            for (int y = 0; y < 2; y++)
                for (int z = 0; z < 2; z++)
                {
                    glm::vec4 pt = invViewProj * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    corners[idx++] = glm::vec3(pt) / pt.w;
                }
        return corners;
    }

    // Small, fixed padding on the X/Y (texel-density-determining) axes, just enough to
    // cover a caster whose center sits right at the frustum edge but whose geometry pokes
    // a little past it. Deliberately NOT m_ShadowQueryRadius-sized - padding X/Y directly
    // widens the box and divides the same tile resolution across more world space, so a
    // large XY margin quietly destroys shadow resolution instead of catching more casters.
    constexpr float kShadowBoxXYMargin = 3.0f;
    // Same reasoning as the XY margin above, applied to depth: catches casters just beyond
    // the visible frustum's near/far without blowing the box's Z range out to hundreds of
    // units. A too-large Z margin doesn't cost texel density, but it does wreck the shadow
    // map's DEPTH PRECISION (the whole range gets crammed into the same fixed-point depth
    // buffer), which shows up as false self-shadowing/acne once the existing polygon-offset
    // bias (tuned for the old ~30-unit range) is no longer big enough relative to how
    // coarsely a much larger range quantizes.
    constexpr float kShadowBoxZMargin = 20.0f;

    // Bounding-sphere fit, not a raw AABB of the transformed corners. An AABB's own size
    // depends on the corners' orientation relative to the light's axes, so it changes as
    // the camera rotates in place (no translation needed) - same frustum, different
    // projected extent - which makes the box's size, its texel density, and any bias tuned
    // against that size all drift with camera orientation. A frustum's bounding SPHERE has
    // a radius that depends only on fov/aspect/near/far (all orientation-invariant), so a
    // box sized to that sphere (same half-extent on every axis) stays a constant size no
    // matter which way the camera looks - only its center moves. X/Y center is then snapped
    // to whole shadow-map texels so the box doesn't sub-texel-jitter as the camera moves
    // (standard stable-fit stabilization).
    ShadowBoxBounds computeLightSpaceBounds(
        const std::array<glm::vec3, 8>& corners, const glm::mat4& lightView, float zMargin, int tileSize)
    {
        glm::vec3 centroid(0.0f);
        for (const auto& c : corners) centroid += c;
        centroid /= (float)corners.size();

        float radius = 0.0f;
        for (const auto& c : corners) radius = std::max(radius, glm::length(c - centroid));

        glm::vec3 lc = glm::vec3(lightView * glm::vec4(centroid, 1.0f));

        ShadowBoxBounds b;
        b.minX = lc.x - radius - kShadowBoxXYMargin; b.maxX = lc.x + radius + kShadowBoxXYMargin;
        b.minY = lc.y - radius - kShadowBoxXYMargin; b.maxY = lc.y + radius + kShadowBoxXYMargin;
        b.minZ = lc.z - radius - zMargin;            b.maxZ = lc.z + radius + zMargin;

        if (tileSize > 0)
        {
            float sizeX = b.maxX - b.minX;
            float sizeY = b.maxY - b.minY;
            float texelX = sizeX / (float)tileSize;
            float texelY = sizeY / (float)tileSize;
            if (texelX > 0.0f)
            {
                float centerX = std::floor(((b.minX + b.maxX) * 0.5f) / texelX) * texelX;
                b.minX = centerX - sizeX * 0.5f;
                b.maxX = centerX + sizeX * 0.5f;
            }
            if (texelY > 0.0f)
            {
                float centerY = std::floor(((b.minY + b.maxY) * 0.5f) / texelY) * texelY;
                b.minY = centerY - sizeY * 0.5f;
                b.maxY = centerY + sizeY * 0.5f;
            }
        }
        return b;
    }
}

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

void GL_Deferred_Renderer::receive(ECXCommand& command)
{
    if (command.type == ECXCommandType::GraphicsChangeHDRExposure)
        m_Exposure = std::any_cast<float>(command.args[0]);
    else if (command.type == ECXCommandType::GraphicsToggleDebug)
        m_DebugRenderer.toggle();
    else if (command.type == ECXCommandType::GraphicsShowDebugRay)
        m_DebugRenderer.showRay(std::any_cast<DebugRayVisualization>(command.args[0]));
    else if (command.type == ECXCommandType::GraphicsShowDebugCone)
        m_DebugRenderer.showCone(std::any_cast<DebugConeVisualization>(command.args[0]));
}

void GL_Deferred_Renderer::init(std::shared_ptr<Window> window, ECXMessenger& messenger, const RenderConfig& config)
{
    messenger.Subscribe(*this, ECXCommandType::GraphicsChangeHDRExposure);
    messenger.Subscribe(*this, ECXCommandType::GraphicsToggleDebug);
    messenger.Subscribe(*this, ECXCommandType::GraphicsShowDebugRay);
    messenger.Subscribe(*this, ECXCommandType::GraphicsShowDebugCone);

    m_Messenger = &messenger;
    m_Window = window;
    m_RenderConfig = config;
    m_Exposure = config.exposure;
    m_DebugRenderer.init(window);
    m_UIRenderer.init(window);
    m_SkyboxRenderer.init(window);

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
    if (!m_LightPassShader->loadShader("data/assets/shaders/lightpass.vert", "data/assets/shaders/lightpass.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load light pass shader", LOGGING::LogLevel::CRITICAL);
    if (!m_ShadowShader.loadShader("data/assets/shaders/shadow.vert", "data/assets/shaders/shadow.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load point shadow pass shader", LOGGING::LogLevel::CRITICAL);
    if (!m_ShadowDirLightShader.loadShader("data/assets/shaders/ShadowLightPass.vert", "data/assets/shaders/DirLightShadowPBR.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load directional shadow pass shader", LOGGING::LogLevel::CRITICAL);
    if (!m_ShadowSpotLightShader.loadShader("data/assets/shaders/ShadowLightPass.vert", "data/assets/shaders/SpotlightShadowPBR.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load spot shadow pass shader", LOGGING::LogLevel::CRITICAL);
    if (!m_PointShadowDepthShader.loadShader(
        "data/assets/shaders/point_shadow.vert",
        "data/assets/shaders/point_shadow.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "failed to load point shadow depth shader", LOGGING::LogLevel::CRITICAL);
    if (!m_BloomDownsampleShader.loadShader("data/assets/shaders/final.vert", "data/assets/shaders/bloomDownsample.frag") ||
        !m_BloomUpsampleShader.loadShader("data/assets/shaders/final.vert", "data/assets/shaders/bloomUpsample.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load bloom shader", LOGGING::LogLevel::CRITICAL);
    if (!m_HDRTonemapShader.loadShader("data/assets/shaders/hdr_tonemap.vert", "data/assets/shaders/hdr_tonemap.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("Failed to load HDR tonemap shader", LOGGING::LogLevel::CRITICAL);
    else
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "HDR tonemap shader loaded, handle=" + std::to_string(m_HDRTonemapShader.getShaderHandle()),
            LOGGING::LogLevel::INFORMATION);
    if (!m_ShadowPointLightShader.loadShader(
        "data/assets/shaders/ShadowLightPass.vert",
        "data/assets/shaders/PointShadowLightPass.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "failed to load point shadow light shader", LOGGING::LogLevel::CRITICAL);
    // Issue #28 receivesShadow-exempt shaders: paired with shadow.vert (bare MVP transform,
    // no varyings) since these draw real mesh geometry rather than a full-screen quad.
    if (!m_ExemptDirLightShader.loadShader("data/assets/shaders/shadow.vert", "data/assets/shaders/ShadowExemptDirLightPass.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "failed to load exempt directional shadow shader", LOGGING::LogLevel::CRITICAL);
    if (!m_ExemptSpotLightShader.loadShader("data/assets/shaders/shadow.vert", "data/assets/shaders/ShadowExemptSpotLightPass.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "failed to load exempt spot shadow shader", LOGGING::LogLevel::CRITICAL);
    if (!m_ExemptPointLightShader.loadShader("data/assets/shaders/shadow.vert", "data/assets/shaders/ShadowExemptPointLightPass.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "failed to load exempt point shadow shader", LOGGING::LogLevel::CRITICAL);
    if (!m_EmissiveShader.loadShader("data/assets/shaders/lightpass.vert", "data/assets/shaders/emissivePass.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "failed to load emissive pass shader", LOGGING::LogLevel::CRITICAL);

    m_PointShadowPool.init(config.pointShadowPoolSize, config.pointShadowFaceSize);
    m_FrameBuffer.init(window->getWidth(), window->getHeight(), config.bloomMipLevels);
    m_LightBuffer.init((*m_LightPassShader));
    m_ShadowAtlas.init(config.shadowAtlasSize, config.shadowAtlasTileSize);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GL_Deferred_Renderer::renderScene(EC_GameScene& scene)
{
    m_FrameBuffer.initFrame();
    geometryPass(scene);
    glowPass();
    lightPass(scene);
    emissivePass();
    hdrPass();
    finalPass();
    skyboxPass(scene);
    debugPass(scene);
    uiPass(scene);
}

void GL_Deferred_Renderer::emissivePass()
{
    m_EmissiveShader.activate();
    m_FrameBuffer.LightingPass(m_EmissiveShader);
    m_EmissiveShader.setUniform("intensity", m_RenderConfig.emissiveIntensity);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    renderQuad();
    glDisable(GL_BLEND);
}

void GL_Deferred_Renderer::hdrPass()
{
    m_HDRTonemapShader.activate();
    m_FrameBuffer.PostProcessPass();
    m_HDRTonemapShader.setUniform("exposure", m_Exposure);
    renderQuad();
}

void GL_Deferred_Renderer::skyboxPass(EC_GameScene& scene)
{
    auto& manager = EC_DOD_EntityManager::getInstance();

    for (EntityID cameraID : scene.getCameras()) {
        if (!manager.isAlive(cameraID)) continue;
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);
        if (!camera.isActive) continue;

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov),
            (float)m_Window->getWidth() / m_Window->getHeight(),
            camera.nearPlane,
            camera.farPlane);

        m_FrameBuffer.SkyboxPass();
        m_SkyboxRenderer.render(camera.viewMatrix, projection);
        break;
    }
}

void GL_Deferred_Renderer::changeResolution(int width, int height)
{
    m_FrameBuffer.resize(width, height);
}

std::vector<EntityID> GL_Deferred_Renderer::queryVisibleEntities(const glm::mat4& viewProjection)
{
    if (!m_Messenger) return {};

    ECXRequest request;
    request.type = ECXRequestType::FrustumCheck;
    request.args[0] = viewProjection;
    request.args[1] = static_cast<uint32_t>(CollisionLayers::Renderable);

    ECXResponse response;
    m_Messenger->publish(request, response);

    if (response.response != ECXResponseType::Success || response.responseData.empty())
        return {};

    try {
        return std::any_cast<std::vector<EntityID>>(response.responseData[0]);
    }
    catch (const std::bad_any_cast&) {
        return {};
    }
}

std::vector<EntityID> GL_Deferred_Renderer::queryEntitiesNear(const glm::vec3& position, float radius)
{
    if (!m_Messenger) return {};

    ECXRequest request;
    request.type = ECXRequestType::EntitySearch;
    request.args[0] = position;
    request.args[1] = radius;
    request.args[2] = static_cast<uint32_t>(CollisionLayers::Renderable);

    ECXResponse response;
    m_Messenger->publish(request, response);

    if (response.response != ECXResponseType::Success || response.responseData.empty())
        return {};

    try {
        return std::any_cast<std::vector<EntityID>>(response.responseData[0]);
    }
    catch (const std::bad_any_cast&) {
        return {};
    }
}

std::vector<EntityID> GL_Deferred_Renderer::queryEntitiesInCone(const glm::vec3& apex, const glm::vec3& direction,
    float halfAngleRadians, float maxDistance)
{
    if (!m_Messenger) return {};

    ECXRequest request;
    request.type = ECXRequestType::ConeCheck;
    request.args[0] = apex;
    request.args[1] = direction;
    request.args[2] = halfAngleRadians;
    request.args[3] = maxDistance;
    request.args[4] = static_cast<uint32_t>(CollisionLayers::Renderable);
    request.args[5] = true;  // castsShadowOnly
    request.args[6] = false; // checkOcclusion - a caster behind another still needs to
                              // render into the depth map, unlike a visibility/interaction
                              // cone check where an occluded candidate should be excluded.

    ECXResponse response;
    m_Messenger->publish(request, response);

    if (response.response != ECXResponseType::Success || response.responseData.empty())
        return {};

    try {
        const auto& hits = std::any_cast<std::vector<RayQueryHit>>(response.responseData[0]);
        std::vector<EntityID> result;
        result.reserve(hits.size());
        for (const auto& hit : hits)
            result.push_back(hit.entity);
        return result;
    }
    catch (const std::bad_any_cast&) {
        return {};
    }
}

void GL_Deferred_Renderer::geometryPass(EC_GameScene& scene)
{
    auto& manager = EC_DOD_EntityManager::getInstance();

    for (EntityID cameraID : scene.getCameras()) {
        if (!manager.isAlive(cameraID)) continue;
        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(cameraID);
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);
        if (!camera.isActive) continue;

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov),
            (float)m_Window->getWidth() / m_Window->getHeight(),
            camera.nearPlane,
            camera.farPlane);
        glm::mat4 view = camera.viewMatrix;

        // Camera-visible set, via the collision system's spatial index (EC_BroadPhase)
        // rather than the renderer maintaining its own structure.
        m_VisibleEntities = queryVisibleEntities(projection * view);

        m_FrameBuffer.GeometryPass();
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        for (EntityID entityID : m_VisibleEntities) {
            if (!manager.isAlive(entityID)) continue;
            if (!manager.hasComponent<EC_DOD_Transform>(entityID)) continue;
            if (!manager.hasComponent<EC_DOD_GraphicsData>(entityID)) continue;

            const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
            auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);

            if (!gfx.visible || !gfx.model || !gfx.shader) continue;

            gfx.shader->activate();
            gfx.shader->setUniform("camPos", spatial.position);
            gfx.shader->setUniform("ViewTransform", view);
            gfx.shader->setUniform("ProjTransform", projection);
            gfx.shader->setUniform("ModelTransform", transform.matrix);
            gfx.shader->setUniform("emissiveIntensity", gfx.emissiveIntensity);

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

void GL_Deferred_Renderer::lightPass(EC_GameScene& scene)
{
    if (!m_LightPassShader) return;
    auto& manager = EC_DOD_EntityManager::getInstance();

    for (EntityID cameraID : scene.getCameras()) {
        if (!manager.isAlive(cameraID)) continue;
        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(cameraID);
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);
        if (!camera.isActive) continue;

        updateLights(scene);

        m_LightPassShader->activate();
        m_FrameBuffer.LightingPass(*m_LightPassShader);
        m_LightBuffer.updatePointLights((int)m_Points.size(), m_Points.data());
        m_LightBuffer.updateSpotLights((int)m_Spots.size(), m_Spots.data());
        m_LightBuffer.bindPointLights();
        m_LightPassShader->setUniform("NumPoints", (int)m_Points.size());
        m_LightBuffer.bindSpotLights();
        m_LightPassShader->setUniform("NumSpots", (int)m_Spots.size());
        m_LightPassShader->setUniform("WSCamPos", spatial.position);

        if (!m_Directionals.empty())
            m_LightPassShader->setDirLight("dirLight", m_Directionals[0]);
        else {
            // setLight("dirLight", ...) previously used here only accepts LightData, and
            // DirLightData's `direction` field (added by inheriting from LightData - see
            // LightData.h) was silently sliced off by that call, so this path's
            // lightpass.frag dirLight.direction uniform was never actually set - every
            // scene until now used a shadow-casting directional light (a different code
            // path, DirLightShadowPBR.frag via setDirLight elsewhere), so this never
            // surfaced. setDirLight sets colour/intensity/direction, matching
            // lightpass.frag's DirLightData uniform exactly.
            DirLightData noDirLight;
            noDirLight.colour = glm::vec4(0.0f);
            noDirLight.position = glm::vec4(0.0f);
            noDirLight.attenuation = glm::vec4(0.0f);
            noDirLight.intensity = 0.0f;
            noDirLight.padding = glm::vec3(0.0f);
            noDirLight.addPadding = glm::vec4(0.0f);
            noDirLight.direction = glm::vec4(0.0f);
            m_LightPassShader->setDirLight("dirLight", noDirLight);
        }

        renderQuad();
        shadowLightingPass(scene);
        break;
    }
}

void GL_Deferred_Renderer::updateLights(EC_GameScene& scene)
{
    m_Directionals.clear();
    m_Spots.clear();
    m_Points.clear();
    m_ShadowDirs.clear();
    m_ShadowSpots.clear();
    m_ShadowPoints.clear();
    m_ShadowPointRadii.clear();
    m_ShadowDirIDs.clear();
    m_ShadowSpotIDs.clear();
    m_ShadowPointIDs.clear();

    auto& manager = EC_DOD_EntityManager::getInstance();

    for (EntityID entityID : scene.getLights()) {
        if (!manager.isAlive(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_Light>(entityID)) continue;
        // A deactivated light (EntityAPI::deactivate(), e.g. a debug light-cycling script)
        // must stop contributing entirely, and so must one whose scene is no longer active
        // - matches the same active+sceneActive check EC_BroadPhase's broad-phase build
        // already does. See EC_DOD_EntityInfo's comment for why they're separate flags.
        if (manager.hasComponent<EC_DOD_EntityInfo>(entityID)) {
            const auto& info = manager.getComponent<EC_DOD_EntityInfo>(entityID);
            if (!info.active || !info.sceneActive) continue;
        }

        const auto& light = manager.getComponent<EC_DOD_Light>(entityID);

        if (light.type == EC_DOD_Light::Type::Directional) {
            DirLightData data;
            data.direction = glm::vec4(light.direction, 0.0f);
            data.colour = glm::vec4(light.colour, 1.0f);
            data.intensity = light.intensity;
            if (!light.castsShadow) m_Directionals.push_back(data);
            else {
                m_ShadowDirs.push_back(data);
                m_ShadowDirIDs.push_back(entityID);
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
            if (!light.castsShadow) m_Spots.push_back(data);
            else {
                m_ShadowSpots.push_back(data);
                m_ShadowSpotIDs.push_back(entityID);
            }
        }
        else {
            LightData data;
            data.position = glm::vec4(light.position, 1.0f);
            data.colour = glm::vec4(light.colour, 1.0f);
            data.intensity = light.intensity;
            data.attenuation = glm::vec4(light.attenuation, 0.0f);
            if (!light.castsShadow) m_Points.push_back(data);
            else {
                m_ShadowPoints.push_back(data);
                m_ShadowPointRadii.push_back(light.cutoffRadius);
                m_ShadowPointIDs.push_back(entityID);
            }
        }
    }
}

void GL_Deferred_Renderer::bakeStaticShadows(EC_GameScene& scene)
{
    updateLights(scene);

    auto& manager = EC_DOD_EntityManager::getInstance();
    // Baking runs right at scene-load completion, potentially before the collision
    // system's spatial index has processed the newly-loaded entities on its own thread -
    // use the full scene entity list rather than a spatial query, since correctness
    // matters more than efficiency for a one-time operation.
    std::vector<EntityID> allEntities = scene.getEntities();

    // A static (dynamic=false) directional light's box is fit once, here, to wherever the
    // scene's active camera is at load time - then never touched again (see the
    // "Directional shadow: camera-following frustum fit" plan's Dynamic-flag contract:
    // static means baked once and trusted forever, not kept in sync with a roaming camera).
    bool haveCamera = false;
    glm::mat4 camView(1.0f);
    glm::vec3 camPos(0.0f);
    float camFov = 60.0f, camNear = 1.0f, aspect = 1.0f;
    for (EntityID cameraID : scene.getCameras())
    {
        if (!manager.isAlive(cameraID)) continue;
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);
        if (!camera.isActive) continue;
        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(cameraID);
        camView = camera.viewMatrix;
        camPos = spatial.position;
        camFov = camera.fov;
        camNear = camera.nearPlane;
        aspect = (float)m_Window->getWidth() / m_Window->getHeight();
        haveCamera = true;
        break;
    }

    if (haveCamera)
    {
        for (size_t i = 0; i < m_ShadowDirs.size(); i++)
        {
            EntityID id = m_ShadowDirIDs[i];
            if (m_BakedStaticDirLights.count(id)) continue;
            if (!manager.isAlive(id) || !manager.hasComponent<EC_DOD_Light>(id)) continue;
            if (manager.getComponent<EC_DOD_Light>(id).dynamic) continue;

            glm::mat4 shadowTransform;
            ShadowBoxBounds bounds;
            int drawn = shadowDirPass(id, m_ShadowDirs[i], allEntities, camView, camPos, camFov, aspect, camNear, shadowTransform, bounds);
            // bakeStaticShadows() runs exactly once per scene (EC_SceneManager, right as it
            // finishes loading) - permanently caching a bake where nothing actually drew
            // (mesh handles not finalized yet, this early after scene load) would leave the
            // light silently unshadowed forever with no way to retry. Leaving it out of the
            // baked set here means shadowLightingPass()'s per-frame loop falls through to the
            // ordinary dynamic-light soft-bake path instead (which does retry every frame) -
            // effectively degrading a failed static bake to dynamic rather than losing the
            // shadow outright.
            if (m_ShadowAtlas.hasTile(id) && drawn > 0)
            {
                m_BakedStaticDirLights.insert(id);
                m_BakedDirShadowTransforms[id] = shadowTransform;
                m_BakedDirShadowBounds[id] = bounds;
            }
        }
    }

    for (size_t i = 0; i < m_ShadowSpots.size(); i++)
    {
        EntityID id = m_ShadowSpotIDs[i];
        if (m_BakedStaticSpotLights.count(id)) continue;
        if (!manager.isAlive(id) || !manager.hasComponent<EC_DOD_Light>(id)) continue;
        if (manager.getComponent<EC_DOD_Light>(id).dynamic) continue;

        glm::mat4 shadowTransform;
        int drawn = shadowSpotPass(id, m_ShadowSpots[i], allEntities, shadowTransform);
        if (m_ShadowAtlas.hasTile(id) && drawn > 0)
        {
            m_BakedStaticSpotLights.insert(id);
            m_BakedSpotShadowTransforms[id] = shadowTransform;
        }
    }

    for (size_t i = 0; i < m_ShadowPoints.size(); i++)
    {
        EntityID id = m_ShadowPointIDs[i];
        if (m_BakedStaticPointLights.count(id)) continue;
        if (!manager.isAlive(id) || !manager.hasComponent<EC_DOD_Light>(id)) continue;
        if (manager.getComponent<EC_DOD_Light>(id).dynamic) continue;

        shadowPointPass(id, m_ShadowPoints[i], allEntities);
        if (m_PointShadowPool.hasSlot(id))
        {
            m_BakedStaticPointLights.insert(id);
        }
    }
}

int GL_Deferred_Renderer::renderShadowCasters(const glm::mat4& view, const glm::mat4& projection, const std::vector<EntityID>& entities)
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    int drawn = 0;

    for (EntityID entityID : entities) {
        if (!manager.isAlive(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_Transform>(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_GraphicsData>(entityID)) continue;

        const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
        const auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);
        if (gfx.getMeshHandle() == 0) continue;
        if (!gfx.castsShadow) continue;

        m_ShadowShader.activate();
        m_ShadowShader.setUniform("ViewTransform", view);
        m_ShadowShader.setUniform("ProjTransform", projection);
        m_ShadowShader.setUniform("ModelTransform", transform.matrix);

        glBindVertexArray(gfx.getMeshHandle());
        glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        drawn++;
    }

    return drawn;
}

int GL_Deferred_Renderer::shadowDirPass(EntityID lightID, DirLightData& light, const std::vector<EntityID>& entities,
    const glm::mat4& camView, const glm::vec3& camPos, float camFov, float aspect, float camNear,
    glm::mat4& outShadowTransform, ShadowBoxBounds& outBounds)
{
    if (!m_ShadowAtlas.acquireTile(lightID)) return 0;

    glm::vec3 eye = glm::vec3(-light.direction);
    glm::vec3 up;
    up[0] = eye[1] - eye[2];
    up[1] = eye[2] - eye[0];
    up[2] = eye[0] - eye[1];

    // Eye is offset from the camera (not the world origin) along the light's direction -
    // only fixes *where* the view matrix is anchored, not its orientation, which still
    // comes entirely from `eye`/`up` as before. See "Directional shadow: camera-following
    // frustum fit" plan - the old glm::lookAt(eye, glm::vec3(0.0f), up) anchored the whole
    // frustum at the world origin regardless of where the camera or its content actually was.
    glm::mat4 view = glm::lookAt(camPos + eye * m_ShadowQueryRadius, camPos, up);

    auto corners = computeCameraFrustumCorners(camView, camFov, aspect, camNear, m_RenderConfig.dirShadowDistance);
    ShadowBoxBounds bounds = computeLightSpaceBounds(corners, view, kShadowBoxZMargin, m_ShadowAtlas.getTileSize());
    outBounds = bounds;

    // glm::lookAt's view space has -Z forward, so a closer point has a larger (less
    // negative) view-space Z - glm::ortho's near/far are positive distances along -Z, so
    // near = -maxZ (closest) and far = -minZ (farthest).
    glm::mat4 projection = glm::ortho(bounds.minX, bounds.maxX, bounds.minY, bounds.maxY, -bounds.maxZ, -bounds.minZ);
    outShadowTransform = m_ShadowAtlas.getTileBiasMatrix(lightID) * projection * view;

    // Bias lives entirely on the READ side now (DirLightShadowPBR.frag's computeOcclusion,
    // slope-scaled against the receiving surface - see learnopengl.com/Advanced-Lighting/
    // Shadows/Shadow-Mapping, which uses no glPolygonOffset at all for exactly this reason).
    // No write-side glPolygonOffset here, and no front-face culling either - front-face
    // culling is ITSELF a (cruder, geometric) bias technique: it deliberately records the
    // far/back surface instead of the near one as an implicit offset. Stacking it on top of
    // the new read-side bias is the same double-biasing mistake as the glPolygonOffset one,
    // and the reference tutorial explicitly warns front-face culling "may still give
    // incorrect results" for a caster close to its receiver - exactly the contact-point gap
    // seen here. Render normally (GL_BACK, the frame's default) and let the read-side bias
    // be the only bias mechanism.
    m_ShadowAtlas.bindTileForWriting(lightID);

    int drawn = renderShadowCasters(view, projection, entities);

    glUseProgram(0);
    m_ShadowAtlas.unbindTileForWriting();
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
    return drawn;
}

int GL_Deferred_Renderer::shadowSpotPass(EntityID lightID, SpotLightData& light, const std::vector<EntityID>& entities, glm::mat4& outShadowTransform)
{
    if (!m_ShadowAtlas.acquireTile(lightID)) return 0;

    glm::vec3 position = light.position;
    glm::vec3 direction = light.direction;
    glm::vec3 up;
    up[0] = direction[1] - direction[2];
    up[1] = direction[2] - direction[0];
    up[2] = direction[0] - direction[1];

    glm::mat4 view = glm::lookAt(position, position + direction, up);
    // FOV must match the light's own cone (cutoffAngle is the HALF-angle, in
    // radians - see EC_DOD_EntityFactory::parseLight), not a fixed guess -
    // otherwise a wider spotlight's shadow map misses the edges of its own
    // visible cone while a narrower one wastes resolution on unlit area.
    glm::mat4 projection = glm::perspective(2.0f * light.cutoffAngle, 1.0f, m_SpotShadowNearPlane, m_SpotShadowFarPlane);
    outShadowTransform = m_ShadowAtlas.getTileBiasMatrix(lightID) * projection * view;

    // No front-face culling here (unlike the original) - for a near-overhead spot and a
    // short caster, the "back" (far-from-light) face front-culling would record is the
    // object's underside, nearly coplanar with the floor it's supposedly occluding -
    // producing almost no usable depth difference to compare against, which reads as no
    // shadow at all. Same lesson as the directional-light fix earlier this session.
    // glPolygonOffset stays - a genuinely different, complementary technique (slope-scaled
    // depth-buffer bias, not "which surface gets recorded").
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0, 2.0);
    m_ShadowAtlas.bindTileForWriting(lightID);

    int drawn = renderShadowCasters(view, projection, entities);

    glUseProgram(0);
    m_ShadowAtlas.unbindTileForWriting();
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
    return drawn;
}

void GL_Deferred_Renderer::shadowPointPass(EntityID lightID, LightData& light, const std::vector<EntityID>& entities)
{
    if (!m_PointShadowPool.acquireSlot(lightID)) return;
    CubemapBuffer& target = m_PointShadowPool.getBuffer(lightID);

    glm::vec3 lightPos = glm::vec3(light.position);

    glm::mat4 projection = glm::perspective(
        glm::radians(kCubemapFaceFovDegrees), 1.0f, m_PointShadowNearPlane, m_PointShadowFarPlane);

    glm::mat4 faceViews[6] = {
        glm::lookAt(lightPos, lightPos + glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(lightPos, lightPos + glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(lightPos, lightPos + glm::vec3(0, 0,-1), glm::vec3(0,-1, 0))
    };

    // No write-side glPolygonOffset here - point_shadow.frag writes gl_FragDepth manually
    // (linear light-distance / FarPlane), which overrides the hardware-rasterized depth
    // glPolygonOffset would have biased, so it had zero effect. No front-face culling
    // either, for the same reason removed from the directional/spot passes: recording the
    // far (light-facing-away) surface instead of the near one is itself a crude geometric
    // bias, and for a caster close to its receiver that gap reads as the shadow detaching
    // from the object. Render normally (GL_BACK, the frame's default) and let the read-side
    // slope-scaled bias in PointShadowLightPass.frag's computeOcclusion be the only bias.
    auto& manager = EC_DOD_EntityManager::getInstance();

    for (int face = 0; face < 6; face++)
    {
        target.bindFace(face);

        for (EntityID entityID : entities)
        {
            if (!manager.isAlive(entityID)) continue;
            if (!manager.hasComponent<EC_DOD_Transform>(entityID)) continue;
            if (!manager.hasComponent<EC_DOD_GraphicsData>(entityID)) continue;

            const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
            const auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);

            if (gfx.getMeshHandle() == 0) continue;
            if (!gfx.castsShadow) continue;

            m_PointShadowDepthShader.activate();
            m_PointShadowDepthShader.setUniform("ModelTransform", transform.matrix);
            m_PointShadowDepthShader.setUniform("ViewTransform", faceViews[face]);
            m_PointShadowDepthShader.setUniform("ProjTransform", projection);
            m_PointShadowDepthShader.setUniform("LightPos", lightPos);
            m_PointShadowDepthShader.setUniform("FarPlane", m_PointShadowFarPlane);

            glBindVertexArray(gfx.getMeshHandle());
            glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glCullFace(GL_BACK);
}

void GL_Deferred_Renderer::exemptDirShadowPass(DirLightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities)
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    bool bound = false;

    for (EntityID entityID : entities) {
        if (!manager.isAlive(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_Transform>(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_GraphicsData>(entityID)) continue;

        const auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);
        if (gfx.getMeshHandle() == 0) continue;
        if (gfx.receivesShadow) continue;

        if (!bound) {
            m_ExemptDirLightShader.activate();
            m_FrameBuffer.LightingPass(m_ExemptDirLightShader);
            m_FrameBuffer.ExemptShadowPass();
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ExemptDirLightShader.setUniform("WSCamPos", camPos);
            m_ExemptDirLightShader.setDirLight("dirLight", light);
            m_ExemptDirLightShader.setUniform("ViewTransform", view);
            m_ExemptDirLightShader.setUniform("ProjTransform", projection);
            bound = true;
        }

        const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
        m_ExemptDirLightShader.setUniform("ModelTransform", transform.matrix);

        glBindVertexArray(gfx.getMeshHandle());
        glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    if (bound) {
        glDisable(GL_BLEND);
        m_FrameBuffer.EndExemptShadowPass();
    }
}

void GL_Deferred_Renderer::exemptSpotShadowPass(SpotLightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities)
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    bool bound = false;

    for (EntityID entityID : entities) {
        if (!manager.isAlive(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_Transform>(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_GraphicsData>(entityID)) continue;

        const auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);
        if (gfx.getMeshHandle() == 0) continue;
        if (gfx.receivesShadow) continue;

        if (!bound) {
            m_ExemptSpotLightShader.activate();
            m_FrameBuffer.LightingPass(m_ExemptSpotLightShader);
            m_FrameBuffer.ExemptShadowPass();
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ExemptSpotLightShader.setUniform("WSCamPos", camPos);
            m_ExemptSpotLightShader.setSpotLight("spotLight", light);
            m_ExemptSpotLightShader.setUniform("ViewTransform", view);
            m_ExemptSpotLightShader.setUniform("ProjTransform", projection);
            bound = true;
        }

        const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
        m_ExemptSpotLightShader.setUniform("ModelTransform", transform.matrix);

        glBindVertexArray(gfx.getMeshHandle());
        glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    if (bound) {
        glDisable(GL_BLEND);
        m_FrameBuffer.EndExemptShadowPass();
    }
}

void GL_Deferred_Renderer::exemptPointShadowPass(LightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities)
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    bool bound = false;

    for (EntityID entityID : entities) {
        if (!manager.isAlive(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_Transform>(entityID)) continue;
        if (!manager.hasComponent<EC_DOD_GraphicsData>(entityID)) continue;

        const auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entityID);
        if (gfx.getMeshHandle() == 0) continue;
        if (gfx.receivesShadow) continue;

        if (!bound) {
            m_ExemptPointLightShader.activate();
            m_FrameBuffer.LightingPass(m_ExemptPointLightShader);
            m_FrameBuffer.ExemptShadowPass();
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ExemptPointLightShader.setUniform("WSCamPos", camPos);
            m_ExemptPointLightShader.setLight("pointLight", light);
            m_ExemptPointLightShader.setUniform("ViewTransform", view);
            m_ExemptPointLightShader.setUniform("ProjTransform", projection);
            bound = true;
        }

        const auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);
        m_ExemptPointLightShader.setUniform("ModelTransform", transform.matrix);

        glBindVertexArray(gfx.getMeshHandle());
        glDrawElements(GL_TRIANGLES, gfx.getVertexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    if (bound) {
        glDisable(GL_BLEND);
        m_FrameBuffer.EndExemptShadowPass();
    }
}

void GL_Deferred_Renderer::shadowLightingPass(EC_GameScene& scene)
{
    auto& manager = EC_DOD_EntityManager::getInstance();

    for (EntityID cameraID : scene.getCameras())
    {
        if (!manager.isAlive(cameraID)) continue;
        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(cameraID);
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);
        if (!camera.isActive) continue;

        // Same camera matrices geometryPass() used - the exempt pass below depth-tests
        // GL_EQUAL against the G-buffer's depth, which requires bit-for-bit the same
        // projection*view*model result geometryPass wrote, not a separately recomputed one.
        glm::mat4 camProjection = glm::perspective(
            glm::radians(camera.fov),
            (float)m_Window->getWidth() / m_Window->getHeight(),
            camera.nearPlane,
            camera.farPlane);
        glm::mat4 camView = camera.viewMatrix;

        // Shadow casters are never narrowed by camera visibility - an entity fully
        // outside the camera's frustum can still cast a shadow onto something that is
        // visible. Each shadow pass gets only the light's own influence radius.
        // Directional lights have no meaningful world position or cutoff radius, so
        // this queries around the camera instead, purely as a bounding heuristic (not
        // a visibility test). Spot/point lights use their cached per-light cutoff
        // radius (EC_DOD_Light::cutoffRadius) instead.
        std::vector<EntityID> activeAtlasLights;
        activeAtlasLights.insert(activeAtlasLights.end(), m_ShadowDirIDs.begin(), m_ShadowDirIDs.end());
        activeAtlasLights.insert(activeAtlasLights.end(), m_ShadowSpotIDs.begin(), m_ShadowSpotIDs.end());
        for (EntityID evicted : m_ShadowAtlas.reconcile(activeAtlasLights))
        {
            m_BakedStaticDirLights.erase(evicted);
            m_BakedStaticSpotLights.erase(evicted);
            m_BakedDirShadowTransforms.erase(evicted);
            m_BakedDirShadowBounds.erase(evicted);
            m_BakedSpotShadowTransforms.erase(evicted);
        }
        for (EntityID evicted : m_PointShadowPool.reconcile(m_ShadowPointIDs))
        {
            m_BakedStaticPointLights.erase(evicted);
        }

        float aspect = (float)m_Window->getWidth() / m_Window->getHeight();

        auto dirCasters = queryEntitiesNear(spatial.position, m_ShadowQueryRadius);
        for (size_t i = 0; i < m_ShadowDirs.size(); i++)
        {
            EntityID id = m_ShadowDirIDs[i];
            glm::mat4 shadowTransform;
            ShadowBoxBounds activeBounds;
            bool baked = m_BakedStaticDirLights.count(id) != 0;
            if (baked)
            {
                shadowTransform = m_BakedDirShadowTransforms.at(id);
                activeBounds = m_BakedDirShadowBounds.at(id);
            }
            else
            {
                // Always re-render for dynamic=true lights - no soft-bake caching. A caching
                // scheme was attempted here (skip re-render when the camera's view is still
                // covered by the last box and nothing nearby is moving) but proved unsafe in
                // practice: the caster query can stabilize its *count* before the underlying
                // entities are actually fully populated (mesh handles finalize a frame or two
                // behind broad-phase indexing), so a render could report a plausible non-zero
                // "drew something" result while still missing some of the real casters -
                // caching that wrong result then never gets corrected, since nothing in a
                // static scene invalidates it afterward. Unconditional per-frame rendering is
                // always correct; revisit caching later with a more robust readiness check.
                shadowDirPass(id, m_ShadowDirs[i], dirCasters, camView, spatial.position, camera.fov, aspect, camera.nearPlane, shadowTransform, activeBounds);
            }
            if (!m_ShadowAtlas.hasTile(id)) continue; // atlas full, skip this light entirely this frame

            m_ShadowDirLightShader.activate();
            m_FrameBuffer.LightingPass(m_ShadowDirLightShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ShadowDirLightShader.setUniform("WSCamPos", spatial.position);
            m_ShadowDirLightShader.setDirLight("dirLight", m_ShadowDirs[i]);
            m_ShadowDirLightShader.setUniform("ShadowTransform", shadowTransform);
            // Read-side, slope-scaled bias (see DirLightShadowPBR.frag's computeOcclusion) needs
            // to know the box's actual world-space depth span to convert a world-space bias
            // into the right NDC offset - this varies with the camera-fit frustum, unlike the
            // old fixed ~30-unit box, so it can't be a shader constant.
            m_ShadowDirLightShader.setUniform("ShadowDepthRange", activeBounds.maxZ - activeBounds.minZ);
            m_ShadowDirLightShader.bindTexture("shadowMap", 5, m_ShadowAtlas.getDepthTexture());
            renderQuad();
            glDisable(GL_BLEND);

            exemptDirShadowPass(m_ShadowDirs[i], camView, camProjection, spatial.position, dirCasters);
        }

        for (size_t i = 0; i < m_ShadowSpots.size(); i++)
        {
            EntityID id = m_ShadowSpotIDs[i];
            // A spot's influence is a cone, not a sphere - queryEntitiesNear's sphere-center
            // test both over-includes (candidates behind the light within the radius) and
            // under-includes (a large caster whose bounds cross into the cone but whose own
            // center falls outside it). queryEntitiesInCone runs the exact GJK cone-vs-shape
            // test EC_BroadPhase already uses for interaction cone checks (see
            // RayConeClickTest.lua), so partial overlaps are correctly included.
            auto nearby = queryEntitiesInCone(glm::vec3(m_ShadowSpots[i].position), glm::vec3(m_ShadowSpots[i].direction),
                m_ShadowSpots[i].cutoffAngle, m_SpotShadowFarPlane);
            glm::mat4 shadowTransform;
            bool baked = m_BakedStaticSpotLights.count(id) != 0;
            if (baked)
                shadowTransform = m_BakedSpotShadowTransforms.at(id);
            else
            {
                // Always re-render for dynamic=true lights - no soft-bake caching. See the
                // identical note in the directional loop above for why: a caching scheme
                // here proved unsafe in practice (a render can report a plausible non-zero
                // "drew something" result while still missing some real casters, if the
                // caster query's count stabilizes before every entity's mesh handle actually
                // finalizes - and that wrong result then never self-corrects). Unconditional
                // per-frame rendering is always correct; revisit caching later with a more
                // robust readiness check.
                shadowSpotPass(id, m_ShadowSpots[i], nearby, shadowTransform);
            }
            if (!m_ShadowAtlas.hasTile(id)) continue; // atlas full, skip this light entirely this frame

            m_ShadowSpotLightShader.activate();
            m_FrameBuffer.LightingPass(m_ShadowSpotLightShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ShadowSpotLightShader.setUniform("WSCamPos", spatial.position);
            m_ShadowSpotLightShader.setSpotLight("spotLight", m_ShadowSpots[i]);
            m_ShadowSpotLightShader.setUniform("ShadowTransform", shadowTransform);
            m_ShadowSpotLightShader.bindTexture("shadowMap", 5, m_ShadowAtlas.getDepthTexture());
            renderQuad();
            glDisable(GL_BLEND);

            exemptSpotShadowPass(m_ShadowSpots[i], camView, camProjection, spatial.position, nearby);
        }

        // Computed once per light here (not per cubemap face) - shadowPointPass loops
        // this same list across all 6 faces internally, a 6x win over re-scanning the
        // full entity list per face.
        for (size_t i = 0; i < m_ShadowPoints.size(); i++)
        {
            EntityID id = m_ShadowPointIDs[i];
            auto nearby = queryEntitiesNear(glm::vec3(m_ShadowPoints[i].position), m_ShadowPointRadii[i]);
            bool baked = m_BakedStaticPointLights.count(id) != 0;
            if (!baked)
            {
                // Always re-render for dynamic=true lights - no soft-bake caching. See the
                // identical note in shadowLightingPass()'s directional/spot loops for why:
                // a caching scheme here has the same unexercised risk (a render could report
                // success while still missing casters whose mesh handles hadn't finished GPU
                // upload yet, and that wrong result would never self-correct afterward).
                shadowPointPass(id, m_ShadowPoints[i], nearby);
            }
            if (!m_PointShadowPool.hasSlot(id)) continue; // pool full, skip this light entirely this frame

            m_ShadowPointLightShader.activate();
            m_FrameBuffer.LightingPass(m_ShadowPointLightShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            m_ShadowPointLightShader.setUniform("WSCamPos", spatial.position);
            m_ShadowPointLightShader.setLight("pointLight", m_ShadowPoints[i]);
            m_ShadowPointLightShader.setUniform("FarPlane", m_PointShadowFarPlane);

            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_PointShadowPool.getTexture(id));
            m_ShadowPointLightShader.setUniform("shadowMap", 5);

            renderQuad();
            glDisable(GL_BLEND);

            exemptPointShadowPass(m_ShadowPoints[i], camView, camProjection, spatial.position, nearby);
        }

        break;
    }

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GL_Deferred_Renderer::postProcess()
{
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

void GL_Deferred_Renderer::glowPass()
{
    m_FrameBuffer.BeginBloomChain();
    int levels = m_FrameBuffer.getBloomLevelCount();

    m_BloomDownsampleShader.activate();
    for (int i = 0; i < levels; i++) {
        m_BloomDownsampleShader.bindTexture("sourceMap", 0, m_FrameBuffer.getBloomDownsampleSource(i));
        m_FrameBuffer.BindBloomDownsampleTarget(i);
        renderQuad();
    }

    m_BloomUpsampleShader.activate();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (int i = levels - 2; i >= 0; i--) {
        m_BloomUpsampleShader.bindTexture("sourceMap", 0, m_FrameBuffer.getBloomUpsampleSource(i));
        m_FrameBuffer.BindBloomUpsampleTarget(i);
        renderQuad();
    }
    glDisable(GL_BLEND);

    m_FrameBuffer.EndBloomChain();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GL_Deferred_Renderer::finalPass()
{
    m_FrameBuffer.FinalPass();
}

void GL_Deferred_Renderer::renderQuad()
{
    glBindVertexArray(m_FS_QuadHandle);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GL_Deferred_Renderer::debugPass(EC_GameScene& scene)
{
    if (!m_DebugRenderer.hasVisualization()) return;
    auto& manager = EC_DOD_EntityManager::getInstance();

    for (EntityID cameraID : scene.getCameras()) {
        if (!manager.isAlive(cameraID)) continue;
        const auto& camera = manager.getComponent<EC_DOD_Camera>(cameraID);
        if (!camera.isActive) continue;

        glm::mat4 projection = glm::perspective(
            glm::radians(camera.fov),
            (float)m_Window->getWidth() / m_Window->getHeight(),
            camera.nearPlane,
            camera.farPlane);

        m_DebugRenderer.render(camera.viewMatrix, projection);
        break;
    }
}

void GL_Deferred_Renderer::uiPass(EC_GameScene& scene)
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    auto entities = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_UI_Element))
        });

    entities.erase(std::remove_if(entities.begin(), entities.end(),
        [&manager](EntityID entity) {
            return !manager.getComponent<EC_UI_Element>(entity).visible;
        }), entities.end());

    std::sort(entities.begin(), entities.end(),
        [&manager](EntityID a, EntityID b) {
            return manager.getComponent<EC_UI_Element>(a).layer < manager.getComponent<EC_UI_Element>(b).layer;
        });

    m_FrameBuffer.UIPass();
    m_UIRenderer.beginFrame();

    for (EntityID entity : entities)
    {
        const auto& element = manager.getComponent<EC_UI_Element>(entity);
        glm::vec2 pos = ResolveUIAbsolutePosition(entity);

        if (manager.hasComponent<EC_UI_Panel>(entity))
        {
            const auto& panel = manager.getComponent<EC_UI_Panel>(entity);
            m_UIRenderer.drawQuad(pos.x, pos.y, element.size.x, element.size.y, panel.colour);
        }
        if (manager.hasComponent<EC_UI_Text>(entity))
        {
            const auto& text = manager.getComponent<EC_UI_Text>(entity);
            m_UIRenderer.drawText(text.text, pos.x, pos.y, text.colour);
        }
    }

    m_UIRenderer.endFrame();
}