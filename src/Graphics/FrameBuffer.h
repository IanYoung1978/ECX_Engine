#pragma once
#include "FrameBufferType.h"
#include <vector>
#include<GL\glew.h>


class FrameBuffer
{
public:
	FrameBuffer();
	bool init(int width, int height);
	void resize(int width, int height);
	void initFrame();
	void setForWriting();
	void setForReading();
	void bindTextures();
	unsigned int getBufferTexture();
	~FrameBuffer();

private:
	unsigned int m_BufferHandle;
	unsigned int m_RenderTexture;
	int m_Width;
	int m_Height;
};

