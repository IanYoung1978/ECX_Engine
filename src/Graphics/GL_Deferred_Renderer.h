#pragma once
#include "Renderer.h"
#include "FrameBufferSet.h"
#include "LightUniformBuffer.h"
#include "ShadowBuffer.h"
#include "CubemapBuffer.h"
#include "Shader.h"
#include "ShadowData.h"
#include "GL_DebugRenderer.h"
#include "Entity/EC_DOD_EntityManager.h"
#include <memory>
#include <vector>
#include <mutex>
#include <glm/glm.hpp>
#include "GL_SkyboxRenderer.h"
#include "Messaging/ICommandListener.h"

enum class PostProcess {
    MSAA,
    HDR,
    DOF,
    NumProcesses
};

class EC_GameScene;

class GL_Deferred_Renderer : public Renderer, public ICommandListener {
public:
    GL_Deferred_Renderer();
    virtual ~GL_Deferred_Renderer();
    virtual void init(std::shared_ptr<Window> window, ECXMessenger& messenger);
    virtual void receive(ECXCommand& command) override;
    virtual void renderScene(EC_GameScene& scene) override;
    virtual void changeResolution(int width, int height) override;
    void setExposure(float exposure) { m_Exposure = exposure; }
    float getExposure() const { return m_Exposure; }

private:
    void geometryPass(EC_GameScene& scene);
    void lightPass(EC_GameScene& scene);
    void updateLights(EC_GameScene& scene);
    void shadowDirPass(ShadowBuffer& target, DirLightData& light, EC_GameScene& scene);
    void shadowSpotPass(ShadowBuffer& target, SpotLightData& light, EC_GameScene& scene);
    void shadowPointPass(CubemapBuffer& target, LightData& light, EC_GameScene& scene);
    void shadowLightingPass(EC_GameScene& scene);
    void skyboxPass(EC_GameScene& scene);
    void debugPass(EC_GameScene& scene);
    void postProcess();
    void glowPass();
    void renderQuad();
    void finalPass();
    void hdrPass();

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
    Shader m_BloomVShader;
    Shader m_BloomHShader;
    Shader m_PointShadowDepthShader;
    GL_SkyboxRenderer m_SkyboxRenderer;
    Shader m_HDRTonemapShader;
    float m_Exposure = 0.75f;
    ShadowBuffer m_ShadowBuffer;
    glm::mat4 m_ShadowDirMatrix;
    glm::mat4 m_ShadowSpotMatrix;
    ShadowData m_ShadowPointMatrices;
    CubemapBuffer m_PointShadowBuffer;
    float m_PointShadowFarPlane = 100.0f;
    std::vector<DirLightData> m_ShadowDirs;
    std::vector<LightData> m_ShadowPoints;
    std::vector<SpotLightData> m_ShadowSpots;
    GL_DebugRenderer m_DebugRenderer;
};