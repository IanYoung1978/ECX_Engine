#pragma once
#include "Graphics/FrameBuffers/FrameBuffer.h"
#include "Graphics/FrameBuffers/GBuffer.h"
#include "Graphics/FrameBuffers/BufferType.h"
#include "Graphics/Shaders/Shader.h"
#include "Graphics/FrameBuffers/ExemptShadowCompositeBuffer.h"
#include "Graphics/Renderers/BloomChain.h"
class FrameBufferSet
{
public:
	FrameBufferSet();
	bool init(int width, int height, int bloomMipLevels);
	void resize(int width, int height);
	void initFrame();
	void GeometryPass();
	void LightingPass(Shader& shader);
	void SkyboxPass();
	void PostProcessPass();
	void UIPass();
	void FinalPass();
	// Issue #28 receivesShadow-exempt pass: depth-tests (GL_EQUAL) against the G-buffer's
	// own depth and writes into FrameBuffer1's colour texture - the same buffer every
	// LightingPass() call this frame already accumulates lit colour into.
	void ExemptShadowPass();
	void EndExemptShadowPass();
	// Mip-chain bloom (see BloomChain.h). BeginBloomChain() binds the G-buffer for reading so
	// its Glow texture can be used as the level-0 downsample source. getBloomDownsampleSource/
	// getBloomUpsampleSource return the correct source texture for step `level` of each walk.
	// EndBloomChain() copies the final blurred result (level 0, post-upsample) back into the
	// G-buffer's Glow texture - same contract the old GlowPass(..., last=true) had, so nothing
	// downstream (emissivePass()) needs to change.
	void BeginBloomChain();
	int getBloomLevelCount() { return m_BloomChain.getLevelCount(); }
	unsigned int getBloomDownsampleSource(int level);
	unsigned int getBloomUpsampleSource(int level) { return m_BloomChain.getLevelTexture(level + 1); }
	void BindBloomDownsampleTarget(int level) { m_BloomChain.bindDownsampleTarget(level); }
	void BindBloomUpsampleTarget(int level) { m_BloomChain.bindUpsampleTarget(level); }
	void EndBloomChain();
	unsigned int getGBufferDepthTexture() { return m_GBuffer.getDepthTexture(); }
	unsigned int getFrameBuffer1Texture() { return m_FrameBuffer1.getBufferTexture(); }
	~FrameBufferSet();
private:
	GBuffer m_GBuffer;
	FrameBuffer m_FrameBuffer1;
	FrameBuffer m_FrameBuffer2;
	ExemptShadowCompositeBuffer m_ExemptShadowBuffer;
	BloomChain m_BloomChain;
	int m_Width, m_Height;
	bool m_SwapBuffers;
};

