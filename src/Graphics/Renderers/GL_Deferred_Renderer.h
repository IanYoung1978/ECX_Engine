#pragma once
#include "Graphics/Renderers/Renderer.h"
#include "Graphics/FrameBuffers/FrameBufferSet.h"
#include "Graphics/FrameBuffers/ShadowAtlas.h"
#include "Graphics/FrameBuffers/CubemapShadowPool.h"
#include "Graphics/Shaders/Shader.h"
#include "Graphics/Renderers/RenderConfig.h"
#include "Graphics/Renderers/GL_DebugRenderer.h"
#include "Graphics/UI/GL_UIRenderer.h"
#include "Entity/EC_DOD_EntityManager.h"
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Graphics/Renderers/GL_SkyboxRenderer.h"
#include "Messaging/ICommandListener.h"

class EC_GameScene;
class ECXMessenger;

// Light-space AABB of a directional shadow's camera-fitted ortho box, used both to build
// the box itself and (for dynamic=true lights) to test whether a later frame's camera
// frustum still fits inside a previous render - see GL_Deferred_Renderer::shadowDirPass.
struct ShadowBoxBounds {
    float minX = 0.0f, maxX = 0.0f;
    float minY = 0.0f, maxY = 0.0f;
    float minZ = 0.0f, maxZ = 0.0f;
};

class GL_Deferred_Renderer : public Renderer, public ICommandListener {
public:
    GL_Deferred_Renderer();
    virtual ~GL_Deferred_Renderer();
    virtual void init(std::shared_ptr<Window> window, ECXMessenger& messenger, const RenderConfig& config) override;
    virtual void receive(ECXCommand& command) override;
    virtual void renderScene(EC_GameScene& scene) override;
    virtual void changeResolution(int width, int height) override;
    virtual void bakeStaticShadows(EC_GameScene& scene) override;
    void setExposure(float exposure) { m_Exposure = exposure; }
    float getExposure() const { return m_Exposure; }
    // Scales the derived ambient colour (see updateLights()).
    void setAmbientScale(float scale) { m_AmbientScale = scale; }
    float getAmbientScale() const { return m_AmbientScale; }
    virtual bool captureFrame(const std::string& target, std::vector<unsigned char>& outPNGBytes) override;

private:
    void geometryPass(EC_GameScene& scene);
    void lightPass(EC_GameScene& scene);
    // Adds the G-buffer's emissive/glow contribution exactly once per frame, independent
    // of active light count - see emissivePass.frag for why this had to be pulled out of
    // every per-light shader.
    void emissivePass();
    void updateLights(EC_GameScene& scene);
    // Shared caster draw loop for shadowDirPass/shadowSpotPass, which differ only in view/projection.
    // Returns how many casters were actually drawn: a mesh handle can lag the spatial query by a frame
    // or two, so entities.size() alone doesn't mean anything rendered.
    int renderShadowCasters(const glm::mat4& view, const glm::mat4& projection, const std::vector<EntityID>& entities);
    // Builds the ortho box by fitting it to the camera's view frustum (clamped to
    // RenderConfig::dirShadowDistance) rather than a fixed world box. outBounds is the light-space AABB
    // it was built from, so callers can test whether a new frustum is still covered. Returns how many
    // casters were drawn (as renderShadowCasters), or 0 if the tile couldn't be acquired.
    int shadowDirPass(EntityID lightID, DirLightData& light, const std::vector<EntityID>& entities,
        const glm::mat4& camView, const glm::vec3& camPos, float camFov, float aspect, float camNear,
        glm::mat4& outShadowTransform, ShadowBoxBounds& outBounds);
    int shadowSpotPass(EntityID lightID, SpotLightData& light, const std::vector<EntityID>& entities, glm::mat4& outShadowTransform);
    void shadowPointPass(EntityID lightID, LightData& light, const std::vector<EntityID>& entities);
    // Redraws only the receivesShadow==false entities among `entities` with this light's unshadowed
    // contribution, depth-testing GL_EQUAL against the G-buffer. view/projection must be the same
    // matrices geometryPass used, or GL_EQUAL mismatches. No-op if none are exempt.
    void exemptDirShadowPass(DirLightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities);
    void exemptSpotShadowPass(SpotLightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities);
    void exemptPointShadowPass(LightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities);
    void shadowLightingPass(EC_GameScene& scene);
    void skyboxPass(EC_GameScene& scene);
    void debugPass(EC_GameScene& scene);
    void uiPass(EC_GameScene& scene);
    void glowPass();
    void renderQuad();
    void finalPass();
    void hdrPass();

    // Ask the collision system's spatial index for entities, rather than the renderer
    // owning/maintaining its own - decouples rendering from collision internals.
    std::vector<EntityID> queryVisibleEntities(const glm::mat4& viewProjection);
    std::vector<EntityID> queryEntitiesNear(const glm::vec3& position, float radius);
    // Exact GJK cone-vs-shape test (EC_BroadPhase::handleConeCheck), not a bounding-sphere
    // approximation - a spot light's actual influence is a cone, not a sphere, and this
    // returns every caster whose shape overlaps it at all (partial overlap included, not
    // just fully-contained), matching what queryEntitiesNear's sphere-center check missed.
    std::vector<EntityID> queryEntitiesInCone(const glm::vec3& apex, const glm::vec3& direction,
        float halfAngleRadians, float maxDistance);

    ECXMessenger* m_Messenger = nullptr;
    RenderConfig m_RenderConfig;
    // Camera-visible set, computed once in geometryPass() for the gbuffer. Not reused by the shadow
    // passes: a caster outside the frustum can still shadow something visible.
    std::vector<EntityID> m_VisibleEntities;
    // Directional lights have no meaningful world position/radius, so their shadow-
    // caster search is a bounding heuristic around the camera instead of a per-light
    // cutoff radius (see EC_DOD_Light::cutoffRadius, used by spot/point lights).
    float m_ShadowQueryRadius = 100.0f;

    std::mutex m_Lock;
    FrameBufferSet m_FrameBuffer;
    std::shared_ptr<Window> m_Window;
    unsigned int m_FS_QuadHandle;
    unsigned int m_FS_QuadVerts;
    unsigned int m_FS_QuadTex;
    unsigned int m_FS_QuadIndices;
    std::vector<LightData> m_Points;
    std::vector<SpotLightData> m_Spots;
    std::vector<DirLightData> m_Directionals;
    Shader m_ShadowShader;
    Shader m_ShadowDirLightShader;
    Shader m_ShadowSpotLightShader;
    Shader m_ShadowPointLightShader;
    Shader m_NonShadowDirLightShader;
    Shader m_NonShadowSpotLightShader;
    Shader m_NonShadowPointLightShader;
    Shader m_ExemptDirLightShader;
    Shader m_ExemptSpotLightShader;
    Shader m_ExemptPointLightShader;
    Shader m_EmissiveShader;
    // Flat ambient term applied in emissivePass(). The colour is derived each frame in
    // updateLights() as the intensity-weighted average of the active lights (white if there are none);
    // m_AmbientScale (RenderConfig::ambientScale) sets its strength.
    glm::vec3 m_DerivedAmbientColour = glm::vec3(1.0f);
    float m_AmbientScale = 0.15f;
    Shader m_BloomDownsampleShader;
    Shader m_BloomUpsampleShader;
    Shader m_PointShadowDepthShader;
    GL_SkyboxRenderer m_SkyboxRenderer;
    Shader m_HDRTonemapShader;
    float m_Exposure = 0.75f;
    ShadowAtlas m_ShadowAtlas;
    CubemapShadowPool m_PointShadowPool;
    float m_PointShadowNearPlane = 0.1f;
    float m_PointShadowFarPlane = 100.0f;
    float m_SpotShadowNearPlane = 1.0f;
    float m_SpotShadowFarPlane = 100.0f;
    std::vector<DirLightData> m_ShadowDirs;
    std::vector<LightData> m_ShadowPoints;
    std::vector<SpotLightData> m_ShadowSpots;
    // Parallel to m_ShadowDirs/m_ShadowSpots/m_ShadowPoints, populated in updateLights() -
    // the stable per-light identity ShadowAtlas/CubemapShadowPool need for slot assignment
    // (light data structs alone carry no EntityID).
    std::vector<EntityID> m_ShadowDirIDs;
    std::vector<EntityID> m_ShadowSpotIDs;
    std::vector<EntityID> m_ShadowPointIDs;

    // Static-light shadow baking (EC_DOD_Light::dynamic == false): a static light's depth is rendered
    // once (bakeStaticShadows(), after scene load); shadowLightingPass() then skips its depth re-render,
    // reusing the cached transform (dir/spot) or the pool's cubemap (point). Entries are erased when
    // ShadowAtlas/CubemapShadowPool::reconcile() evicts the light, so a re-acquired slot is never
    // mistaken for old baked data.
    std::unordered_set<EntityID> m_BakedStaticDirLights;
    std::unordered_set<EntityID> m_BakedStaticSpotLights;
    std::unordered_set<EntityID> m_BakedStaticPointLights;
    std::unordered_map<EntityID, glm::mat4> m_BakedDirShadowTransforms;
    std::unordered_map<EntityID, ShadowBoxBounds> m_BakedDirShadowBounds;
    std::unordered_map<EntityID, glm::mat4> m_BakedSpotShadowTransforms;

    // Cutoff radius for each entry in m_ShadowPoints (aligned 1:1), copied from
    // EC_DOD_Light::cutoffRadius in updateLights(). Spot casters use queryEntitiesInCone instead.
    std::vector<float> m_ShadowPointRadii;
    GL_DebugRenderer m_DebugRenderer;
    GL_UIRenderer m_UIRenderer;
};