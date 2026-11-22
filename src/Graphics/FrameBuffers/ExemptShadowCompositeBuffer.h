#pragma once

// Small FBO reusing the G-buffer's depth texture and the lighting accumulation colour texture
// (FrameBuffer1) for the receivesShadow-exempt redraw. bindForWriting() depth-tests GL_EQUAL against
// what geometryPass wrote, so only the exempt entity's own pixels are touched.
class ExemptShadowCompositeBuffer
{
public:
    ExemptShadowCompositeBuffer();
    ~ExemptShadowCompositeBuffer();

    bool init(unsigned int gbufferDepthTexture, unsigned int colourTexture);
    // Must be called whenever gbufferDepthTexture/colourTexture are recreated (e.g. GBuffer/
    // FrameBuffer1 delete-and-reinit their textures on window resize) - otherwise this FBO
    // keeps pointing at stale, deleted texture objects and the next bind fails or renders
    // into nothing.
    void resize(unsigned int gbufferDepthTexture, unsigned int colourTexture);

    void bindForWriting(int width, int height);
    void unbind();

private:
    void attach(unsigned int gbufferDepthTexture, unsigned int colourTexture);

    unsigned int m_FBO = 0;
};
