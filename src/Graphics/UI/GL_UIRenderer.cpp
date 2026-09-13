#include "GL_UIRenderer.h"
#include <GL\glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <algorithm>
#include "Window/Window.h"
#include "Graphics/FrameBuffers/BufferType.h"
#include "Logging/ECX_Logging.h"

void GL_UIRenderer::init(std::shared_ptr<Window> window)
{
    m_Window = window;

    if (!m_Font.load("data/assets/fonts/JetBrainsMono.ttf", kDefaultFontSizePx))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("GL_UIRenderer: failed to load font atlas", LOGGING::LogLevel::CRITICAL);

    if (!m_TextShader.loadShader("data/assets/shaders/ui_text.vert", "data/assets/shaders/ui_text.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("GL_UIRenderer: failed to load text shader", LOGGING::LogLevel::CRITICAL);
    if (!m_QuadShader.loadShader("data/assets/shaders/ui_quad.vert", "data/assets/shaders/ui_quad.frag"))
        LOGGING::ECX_Logger::GetInstance()->LogMessage("GL_UIRenderer: failed to load quad shader", LOGGING::LogLevel::CRITICAL);

    // Dynamic text VAO: pos(vec2) + uv(vec2) per vertex, non-indexed triangles (6 verts/glyph),
    // refilled via glBufferSubData every drawText() call.
    glGenVertexArrays(1, &m_TextVAO);
    glBindVertexArray(m_TextVAO);
    glGenBuffers(1, &m_TextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_TextVBO);
    glBufferData(GL_ARRAY_BUFFER, kMaxTextChars * 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer((GLuint)BufferType::Vertex, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray((GLuint)BufferType::Vertex);
    glVertexAttribPointer((GLuint)BufferType::TextureCoordinate, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray((GLuint)BufferType::TextureCoordinate);
    glBindVertexArray(0);

    // Static unit quad (0,0)-(1,1), position only - reused for every draw via per-draw
    // offset/scale uniforms.
    float quadVerts[] = { 0.0f,0.0f, 1.0f,0.0f, 1.0f,1.0f, 0.0f,0.0f, 1.0f,1.0f, 0.0f,1.0f };
    glGenVertexArrays(1, &m_QuadVAO);
    glBindVertexArray(m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glVertexAttribPointer((GLuint)BufferType::Vertex, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray((GLuint)BufferType::Vertex);
    glBindVertexArray(0);
}

void GL_UIRenderer::beginFrame()
{
    m_Projection = glm::ortho(0.0f, (float)m_Window->getWidth(), (float)m_Window->getHeight(), 0.0f, -1.0f, 1.0f);

    glDisable(GL_DEPTH_TEST);
    // UI's top-left-origin, Y-down projection flips winding relative to normal 3D content -
    // without this, quads/glyphs get back-face culled using whatever cull state 3D passes left
    // behind and render invisibly with no GL error.
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GL_UIRenderer::endFrame()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

float GL_UIRenderer::measureTextWidth(const std::string& text) const
{
    float width = 0.0f;
    for (char c : text)
        width += m_Font.getGlyph(c).xadvance;
    return width;
}

void GL_UIRenderer::drawText(const std::string& text, float x, float y, const glm::vec4& colour)
{
    std::vector<float> vertices;
    vertices.reserve(std::min(text.size(), kMaxTextChars) * 6 * 4);

    float penX = x;
    float penY = y;
    for (char c : text)
    {
        if (c == '\n')
        {
            penX = x;
            penY += getLineHeight();
            continue;
        }
        if (vertices.size() / (6 * 4) >= kMaxTextChars) break;
        const FontAtlas::Glyph& g = m_Font.getGlyph(c);

        float x0 = penX + g.xoff;
        float y0 = penY + g.yoff;
        float x1 = x0 + g.width;
        float y1 = y0 + g.height;

        float quad[] = {
            x0, y0, g.u0, g.v0,
            x1, y0, g.u1, g.v0,
            x1, y1, g.u1, g.v1,
            x0, y0, g.u0, g.v0,
            x1, y1, g.u1, g.v1,
            x0, y1, g.u0, g.v1,
        };
        vertices.insert(vertices.end(), std::begin(quad), std::end(quad));

        penX += g.xadvance;
    }

    if (vertices.empty()) return;

    glBindBuffer(GL_ARRAY_BUFFER, m_TextVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

    m_TextShader.activate();
    m_TextShader.setUniform("Projection", m_Projection);
    m_TextShader.setUniform("textColour", colour);
    m_TextShader.bindTexture("fontAtlas", 0, m_Font.getTextureHandle());

    glBindVertexArray(m_TextVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));
    glBindVertexArray(0);
}

void GL_UIRenderer::drawQuad(float x, float y, float w, float h, const glm::vec4& colour)
{
    m_QuadShader.activate();
    m_QuadShader.setUniform("Projection", m_Projection);
    m_QuadShader.setUniform("offset", glm::vec2(x, y));
    m_QuadShader.setUniform("scale", glm::vec2(w, h));
    m_QuadShader.setUniform("colour", colour);

    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

GL_UIRenderer::~GL_UIRenderer()
{
    if (m_TextVBO) glDeleteBuffers(1, &m_TextVBO);
    if (m_TextVAO) glDeleteVertexArrays(1, &m_TextVAO);
    if (m_QuadVBO) glDeleteBuffers(1, &m_QuadVBO);
    if (m_QuadVAO) glDeleteVertexArrays(1, &m_QuadVAO);
}
