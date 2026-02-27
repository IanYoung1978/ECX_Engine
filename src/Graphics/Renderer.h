#pragma once
#include <memory>
class Window;
class GameEntity;
class Renderer
{
public:
    Renderer();
    virtual void init(std::shared_ptr<Window> window) = 0;
    virtual void changeResolution(int width, int height) = 0;
    virtual void renderScene() = 0;
    virtual void toggleDebug() {}
    virtual ~Renderer();
};