#pragma once
#include <memory>
class Window;
class GameEntity;
class ECXMessenger;
class EC_GameScene;
class Renderer
{
public:
    Renderer();
    virtual void init(std::shared_ptr<Window> window, ECXMessenger& messenger) = 0;
    virtual void changeResolution(int width, int height) = 0;
    virtual void renderScene(EC_GameScene& scene) = 0;
    virtual void toggleDebug() {}
    virtual ~Renderer();
};