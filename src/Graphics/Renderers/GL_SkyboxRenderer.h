#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "Graphics/Shaders/Shader.h"

class Window;

class GL_SkyboxRenderer
{
public:
    GL_SkyboxRenderer() = default;
    ~GL_SkyboxRenderer();
    void init(std::shared_ptr<Window> window);
    void render(const glm::mat4& view, const glm::mat4& projection);

private:
    void buildCubeVAO();

    Shader m_SkyboxShader;
    std::shared_ptr<Window> m_Window;
    unsigned int m_CubeVAO = 0;
    unsigned int m_CubeVBO = 0;
};