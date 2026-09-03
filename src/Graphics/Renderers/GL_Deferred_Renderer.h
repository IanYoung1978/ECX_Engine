#pragma once
#include "Graphics/Renderers/Renderer.h"
#include "Graphics/FrameBuffers/FrameBufferSet.h"
#include "Graphics/Renderers/LightUniformBuffer.h"
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

enum class PostProcess {
    MSAA,
    HDR,
    DOF,
    NumProcesses
};

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

private:
    void geometryPass(EC_GameScene& scene);
    void lightPass(EC_GameScene& scene);
    // Adds the G-buffer's emissive/glow contribution exactly once per frame, independent
    // of active light count - see emissivePass.frag for why this had to be pulled out of
    // every per-light shader.
    void emissivePass();
    void updateLights(EC_GameScene& scene);
    // Shared entity-draw loop used by shadowDirPass/shadowSpotPass (identical body -
    // alive/Transform/GraphicsData/mesh-handle checks + m_ShadowShader uniform sets + draw
    // call - only the view/projection construction differs between the two callers, which
    // they each keep). shadowPointPass's 6-face loop is structurally different enough that
    // forcing it into this helper would hurt readability more than it would help.
    // Returns how many casters were actually drawn (passed every filter: alive, has
    // Transform/GraphicsData, mesh handle finalized, castsShadow) - NOT just entities.size().
    // A caster's mesh handle can still be 0 for a frame or two after it starts appearing in
    // spatial queries (GPU resource finalization lags a frame behind broad-phase indexing),
    // so entities.size() > 0 alone doesn't mean anything was actually rendered - callers
    // that cache a render need this to avoid caching an effectively-empty one.
    int renderShadowCasters(const glm::mat4& view, const glm::mat4& projection, const std::vector<EntityID>& entities);
    // Builds the ortho box by fitting it to the camera's current view frustum (clamped to
    // RenderConfig::dirShadowDistance) instead of a fixed world-space box - see the
    // "Directional shadow: camera-following frustum fit" plan. outBounds is the light-space
    // AABB the box was built from, so callers can later test whether a new camera frustum
    // is still covered by it without re-rendering (the dynamic-light soft-bake check).
    // Return value: same "how many casters were actually drawn" as renderShadowCasters, or
    // 0 if the tile couldn't be acquired.
    int shadowDirPass(EntityID lightID, DirLightData& light, const std::vector<EntityID>& entities,
        const glm::mat4& camView, const glm::vec3& camPos, float camFov, float aspect, float camNear,
        glm::mat4& outShadowTransform, ShadowBoxBounds& outBounds);
    int shadowSpotPass(EntityID lightID, SpotLightData& light, const std::vector<EntityID>& entities, glm::mat4& outShadowTransform);
    void shadowPointPass(EntityID lightID, LightData& light, const std::vector<EntityID>& entities);
    // Issue #28: redraws just the receivesShadow==false entities among `entities` with this
    // light's full (unshadowed) contribution, depth-testing GL_EQUAL against the G-buffer so
    // only their own pixels are touched. view/projection must be the same camera matrices
    // geometryPass used, not recomputed, to avoid a GL_EQUAL floating-point mismatch. No-op
    // if none of `entities` are exempt.
    void exemptDirShadowPass(DirLightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities);
    void exemptSpotShadowPass(SpotLightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities);
    void exemptPointShadowPass(LightData& light, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos, const std::vector<EntityID>& entities);
    void shadowLightingPass(EC_GameScene& scene);
    void skyboxPass(EC_GameScene& scene);
    void debugPass(EC_GameScene& scene);
    void uiPass(EC_GameScene& scene);
    void postProcess();
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
    // Camera-visible set, computed once in geometryPass() - what actually gets drawn
    // to the gbuffer. Deliberately NOT reused by the shadow passes: a shadow caster
    // outside the camera's frustum can still cast a shadow onto something that is
    // visible, so narrowing shadow casters by camera visibility is wrong, not just an
    // optimization detail.
    std::vector<EntityID> m_VisibleEntities;
    // Directional lights have no meaningful world position/radius, so their shadow-
    // caster search is a bounding heuristic around the camera instead of a per-light
    // cutoff radius (see EC_DOD_Light::cutoffRadius, used by spot/point lights).
    float m_ShadowQueryRadius = 100.0f;

    std::mutex m_Lock;
    LightUniformBuffer m_LightBuffer;
    FrameBufferSet m_FrameBuffer;
    std::shared_ptr<Window> m_Window;
    std::shared_ptr<Shader> m_LightPassShader;
    std::vector<std::shared_ptr<Shader>> m_PostProcessShaders;
    unsigned int m_FS_QuadHandle;
    unsigned int m_FS_QuadVerts;
    unsigned int m_FS_QuadTex;
    unsigned int m_FS_QuadIndices;
    bool m_HDR;
    bool m_DOF;
    bool m_MSAA;
    std::vector<LightData> m_Points;
    std::vector<SpotLightData> m_Spots;
    std::vector<DirLightData> m_Directionals;
    Shader m_ShadowShader;
    Shader m_ShadowDirLightShader;
    Shader m_ShadowSpotLightShader;
    Shader m_ShadowPointLightShader;
    Shader m_ExemptDirLightShader;
    Shader m_ExemptSpotLightShader;
    Shader m_ExemptPointLightShader;
    Shader m_EmissiveShader;
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

    // Static-light shadow baking (EC_DOD_Light::dynamic == false): a static light's depth
    // is rendered once (see bakeStaticShadows(), called after scene load) and never again -
    // shadowLightingPass() skips the depth re-render for any light present in these sets,
    // reusing the cached transform (dir/spot) or the pool's already-populated cubemap
    // (point). Entries are erased when ShadowAtlas/CubemapShadowPool::reconcile() evicts
    // that light's tile/slot, so a later re-acquired tile/slot is never mistaken for
    // already-baked data left behind by a different light.
    std::unordered_set<EntityID> m_BakedStaticDirLights;
    std::unordered_set<EntityID> m_BakedStaticSpotLights;
    std::unordered_set<EntityID> m_BakedStaticPointLights;
    std::unordered_map<EntityID, glm::mat4> m_BakedDirShadowTransforms;
    std::unordered_map<EntityID, ShadowBoxBounds> m_BakedDirShadowBounds;
    std::unordered_map<EntityID, glm::mat4> m_BakedSpotShadowTransforms;

    // Cached cutoff radius for each entry in m_ShadowPoints, indices aligned 1:1.
    // Copied from EC_DOD_Light::cutoffRadius in updateLights() - not recomputed here,
    // since the radius only depends on light data set at load time. Spot's equivalent
    // caster query now uses queryEntitiesInCone (the light's own cutoffAngle/far plane),
    // not a radius.
    std::vector<float> m_ShadowPointRadii;
    GL_DebugRenderer m_DebugRenderer;
    GL_UIRenderer m_UIRenderer;
};