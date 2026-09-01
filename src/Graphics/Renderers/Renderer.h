#pragma once
#include <memory>
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
    virtual ~Renderer();
};