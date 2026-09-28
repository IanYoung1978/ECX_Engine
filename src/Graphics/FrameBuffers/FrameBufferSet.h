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
	// receivesShadow-exempt pass: depth-tests (GL_EQUAL) against the G-buffer depth and writes into
	// FrameBuffer1's colour texture, where lighting accumulates.
	void ExemptShadowPass();
	void EndExemptShadowPass();
	// Mip-chain bloom (see BloomChain.h). BeginBloomChain() binds the G-buffer for reading so Glow is
	// the level-0 source; getBloomDownsampleSource/getBloomUpsampleSource give the source texture for
	// step `level`; EndBloomChain() copies the result (level 0) back into the G-buffer's Glow texture.
	void BeginBloomChain();
	int getBloomLevelCount() { return m_BloomChain.getLevelCount(); }
	unsigned int getBloomDownsampleSource(int level);
	unsigned int getBloomUpsampleSource(int level) { return m_BloomChain.getLevelTexture(level + 1); }
	void BindBloomDownsampleTarget(int level) { m_BloomChain.bindDownsampleTarget(level); }
	void BindBloomUpsampleTarget(int level) { m_BloomChain.bindUpsampleTarget(level); }
	void EndBloomChain();
	unsigned int getGBufferDepthTexture() { return m_GBuffer.getDepthTexture(); }
	unsigned int getGBufferTexture(FrameBufferType type) { return m_GBuffer.getGBufferTexture(type); }
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

