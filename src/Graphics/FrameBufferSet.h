#pragma once
#include "FrameBuffer.h"
#include "GBuffer.h"
#include "BufferType.h"
#include "Shader.h"
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
	void PostProcessPass();
	void UIPass();
	void FinalPass();
	~FrameBufferSet();
private:
	GBuffer m_GBuffer;
	FrameBuffer m_FrameBuffer1;
	FrameBuffer m_FrameBuffer2;
	int m_Width, m_Height;
	bool m_SwapBuffers;
};

