#pragma once
#include "Renderer.h"
#include "FrameBufferSet.h"
#include "LightUniformBuffer.h"
#include "ShadowBuffer.h"
#include "ShadowCubeBuffer.h"
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

class GL_Deferred_Renderer : public Renderer, public ICommandListener {
public:
    GL_Deferred_Renderer();
    virtual ~GL_Deferred_Renderer();
    virtual void init(std::shared_ptr<Window> window, ECXMessenger& messenger);
    virtual void receive(ECXCommand& command) override;
    virtual void renderScene() override;
    virtual void changeResolution(int width, int height) override;
    void setExposure(float exposure) { m_Exposure = exposure; }
    float getExposure() const { return m_Exposure; }
private:
    void geometryPass();
    void lightPass();
    void postProcess();
    void glowPass();
    void renderQuad();
    void finalPass();
    void debugPass();
    void updateLights();
    void shadowDirPass(ShadowBuffer& target, DirLightData& light);
    void shadowSpotPass(ShadowBuffer& target, SpotLightData& light);
    void shadowPointPass(ShadowBuffer& target, LightData& light);
    void shadowLightingPass();
    void skyboxPass();
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
    GL_SkyboxRenderer m_SkyboxRenderer;
    Shader m_HDRTonemapShader;
    float m_Exposure = 0.75f;
    ShadowBuffer m_ShadowBuffer;
    glm::mat4 m_ShadowDirMatrix;
    glm::mat4 m_ShadowSpotMatrix;
    ShadowData m_ShadowPointMatrices;
    ShadowCubeBuffer m_ShadowCubeBuffer;
    std::vector<DirLightData> m_ShadowDirs;
    std::vector<LightData> m_ShadowPoints;
    std::vector<SpotLightData> m_ShadowSpots;
    GL_DebugRenderer m_DebugRenderer;
};