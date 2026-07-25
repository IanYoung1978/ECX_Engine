#pragma once
#include "FrameBuffer.h"
#include "GBuffer.h"
#include "BufferType.h"
#include "Shader.h"
#include "ExemptShadowCompositeBuffer.h"
class FrameBufferSet
{
public:
	FrameBufferSet();
	bool init(int width, int height);
	void resize(int width, int height);
	void initFrame();
	void GeometryPass();
	void LightingPass(Shader& shader);
	void GlowPass(Shader& shader, bool first, bool last);
	void SkyboxPass();
	void PostProcessPass();
	void UIPass();
	void FinalPass();
	// Issue #28 receivesShadow-exempt pass: depth-tests (GL_EQUAL) against the G-buffer's
	// own depth and writes into FrameBuffer1's colour texture - the same buffer every
	// LightingPass() call this frame already accumulates lit colour into.
	void ExemptShadowPass();
	void EndExemptShadowPass();
	unsigned int getGBufferDepthTexture() { return m_GBuffer.getDepthTexture(); }
	unsigned int getFrameBuffer1Texture() { return m_FrameBuffer1.getBufferTexture(); }
	~FrameBufferSet();
private:
	GBuffer m_GBuffer;
	FrameBuffer m_FrameBuffer1;
	FrameBuffer m_FrameBuffer2;
	ExemptShadowCompositeBuffer m_ExemptShadowBuffer;
	int m_Width, m_Height;
	bool m_SwapBuffers;
};

