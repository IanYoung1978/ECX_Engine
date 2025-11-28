#pragma once
#include "FrameBufferType.h"
#include <vector>
#include<GL\glew.h>
class GBuffer
{
public:
	GBuffer();
	bool init(int width, int height);
	void resize(int width, int height);
	void initFrame();
	void setForWriting();
	void setForReading();
	unsigned int getGBufferTexture(FrameBufferType type);
	~GBuffer();
private:
	static const int TOTAL_BUFFERS = 5;
	unsigned int m_BufferHandle;
	std::vector<unsigned int> m_Buffers;
	GLenum* m_DrawBuffers;
	unsigned int m_DepthBuffer;
	int m_Width;
	int m_Height;
};

