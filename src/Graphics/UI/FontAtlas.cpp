#include "FontAtlas.h"
#include <GL\glew.h>
#include <fstream>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "Logging/ECX_Logging.h"

bool FontAtlas::load(const std::string& ttfPath, float pixelHeight, int atlasWidth, int atlasHeight)
{
    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("FontAtlas: failed to open " + ttfPath, LOGGING::LogLevel::CRITICAL);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> ttfBuffer((size_t)size);
    if (!file.read(reinterpret_cast<char*>(ttfBuffer.data()), size))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("FontAtlas: failed to read " + ttfPath, LOGGING::LogLevel::CRITICAL);
        return false;
    }

    std::vector<unsigned char> bitmap((size_t)atlasWidth * atlasHeight, 0);
    std::vector<stbtt_bakedchar> bakedChars(96);

    int result = stbtt_BakeFontBitmap(ttfBuffer.data(), 0, pixelHeight, bitmap.data(), atlasWidth, atlasHeight, 32, 96, bakedChars.data());
    if (result <= 0)
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "FontAtlas: atlas overflow baking " + ttfPath + " - increase atlas size or reduce pixel height",
            LOGGING::LogLevel::CRITICAL);
        return false;
    }

    for (int i = 0; i < 96; i++)
    {
        const stbtt_bakedchar& bc = bakedChars[i];
        Glyph& g = m_Glyphs[i];
        g.u0 = (float)bc.x0 / atlasWidth;
        g.v0 = (float)bc.y0 / atlasHeight;
        g.u1 = (float)bc.x1 / atlasWidth;
        g.v1 = (float)bc.y1 / atlasHeight;
        g.width = (float)(bc.x1 - bc.x0);
        g.height = (float)(bc.y1 - bc.y0);
        g.xoff = bc.xoff;
        g.yoff = bc.yoff;
        g.xadvance = bc.xadvance;
    }
    m_FallbackGlyph = m_Glyphs['?' - 32];

    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, ttfBuffer.data(), stbtt_GetFontOffsetForIndex(ttfBuffer.data(), 0));
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    m_LineHeight = (ascent - descent + lineGap) * scale;

    glGenTextures(1, &m_TextureHandle);
    glBindTexture(GL_TEXTURE_2D, m_TextureHandle);
    // Atlas is single-channel (1 byte/pixel) - default 4-byte unpack alignment would skew rows
    // whose width isn't a multiple of 4.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

const FontAtlas::Glyph& FontAtlas::getGlyph(char c) const
{
    int idx = (int)c - 32;
    if (idx < 0 || idx >= 96) return m_FallbackGlyph;
    return m_Glyphs[idx];
}

FontAtlas::~FontAtlas()
{
    if (m_TextureHandle)
        glDeleteTextures(1, &m_TextureHandle);
}
