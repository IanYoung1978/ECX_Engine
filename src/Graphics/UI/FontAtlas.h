#pragma once
#include <string>
#include <vector>
#include <stb_truetype.h>

// Bakes a TrueType font into a single-channel bitmap atlas (ASCII 32-126 only, proportional
// widths) via stb_truetype, once at load time - no runtime re-bake, no Unicode. See the UI
// foundation plan for why: this is enough for a debug telemetry overlay, not a general text
// system.
class FontAtlas
{
public:
    struct Glyph
    {
        float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f; // atlas UV rect
        float width = 0.0f, height = 0.0f;                // glyph quad size in pixels
        float xoff = 0.0f, yoff = 0.0f;                   // pen-to-quad-top-left offset, pixels
        float xadvance = 0.0f;                             // pen advance after this glyph, pixels
    };

    bool load(const std::string& ttfPath, float pixelHeight, int atlasWidth = 512, int atlasHeight = 512);
    const Glyph& getGlyph(char c) const;
    unsigned int getTextureHandle() const { return m_TextureHandle; }
    float getLineHeight() const { return m_LineHeight; }
    ~FontAtlas();

private:
    unsigned int m_TextureHandle = 0;
    Glyph m_Glyphs[96]; // ASCII 32..127
    Glyph m_FallbackGlyph;
    float m_LineHeight = 0.0f;
};
