#pragma once

// A small FBO that reuses the G-buffer's already-populated depth texture and writes into
// the same colour texture the lighting passes accumulate into (FrameBuffer1), just for the
// receivesShadow-exempt redraw pass (Issue #28). Reusing an existing depth texture lets
// bindForWriting() depth-test with GL_EQUAL against exactly what geometryPass already
// wrote, so the exempt pass only ever touches pixels belonging to the exempt entity's own
// geometry - without restructuring GBuffer or FrameBuffer (both used unmodified elsewhere
// in the pipeline).
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
