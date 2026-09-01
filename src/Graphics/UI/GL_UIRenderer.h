#pragma once
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "FontAtlas.h"
#include "Graphics/Shaders/Shader.h"

class Window;

// Generic screen-space (pixel, top-left origin, Y-down) 2D rendering foundation - text via a
// baked font atlas, and flat-colour quads. This is the shared drawing toolkit for ALL UI work
// (Issue #6's scripted game-UI elements, a future debug CLI, etc.), not specific to any one
// feature. Owns no concept of what to draw or when - callers (e.g. a debug telemetry overlay)
// decide content, this class only knows how to put pixels/glyphs on screen.
//
// Usage per frame: beginFrame() once, any number of drawText()/drawQuad() calls, endFrame() once.
class GL_UIRenderer
{
public:
    void init(std::shared_ptr<Window> window);
    void beginFrame();
    void endFrame();

    void drawText(const std::string& text, float x, float y, const glm::vec4& colour);
    void drawQuad(float x, float y, float w, float h, const glm::vec4& colour);
    float measureTextWidth(const std::string& text) const;
    float getLineHeight() const { return m_Font.getLineHeight(); }

    ~GL_UIRenderer();

private:
    std::shared_ptr<Window> m_Window;
    FontAtlas m_Font;
    Shader m_TextShader;
    Shader m_QuadShader;
    glm::mat4 m_Projection{ 1.0f };

    unsigned int m_TextVAO = 0, m_TextVBO = 0;
    static constexpr size_t kMaxTextChars = 4096;
    static constexpr float kDefaultFontSizePx = 16.0f;

    unsigned int m_QuadVAO = 0, m_QuadVBO = 0;
};
