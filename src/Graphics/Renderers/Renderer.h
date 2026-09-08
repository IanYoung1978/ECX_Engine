#pragma once
#include <memory>
#include <vector>
#include "Graphics/Renderers/RenderConfig.h"
class Window;
class GameEntity;
class ECXMessenger;
class EC_GameScene;
class Renderer
{
public:
    Renderer();
    virtual void init(std::shared_ptr<Window> window, ECXMessenger& messenger, const RenderConfig& config) = 0;
    virtual void changeResolution(int width, int height) = 0;
    virtual void renderScene(EC_GameScene& scene) = 0;
    virtual void toggleDebug() {}
    // Render each currently-static (EC_DOD_Light::dynamic == false) shadow-casting light's
    // shadow map exactly once. Called after a scene finishes loading; default no-op for any
    // Renderer implementation that doesn't support shadow baking.
    virtual void bakeStaticShadows(EC_GameScene& scene) {}
    // Encodes the most recently rendered frame as a PNG (see EC_DebugHTTPServer's GET
    // /screenshot) - default no-op/unsupported, matching bakeStaticShadows' convention,
    // since this is a GL_Deferred_Renderer-specific capability. Must be called from the
    // GL/main thread - this does real glReadPixels work, not just data access.
    virtual bool captureFrame(std::vector<unsigned char>& outPNGBytes) { return false; }
    virtual ~Renderer();
};