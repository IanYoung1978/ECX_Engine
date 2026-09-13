#pragma once

class CubemapBuffer
{
public:
    CubemapBuffer();
    ~CubemapBuffer();

    bool init(int width, int height);
    void bindFace(int face);
    unsigned int getTexture() const { return m_Texture; }
    int getWidth()  const { return m_Width; }
    int getHeight() const { return m_Height; }

private:
    unsigned int m_FBO = 0;
    unsigned int m_Texture = 0;
    int m_Width = 0;
    int m_Height = 0;
};