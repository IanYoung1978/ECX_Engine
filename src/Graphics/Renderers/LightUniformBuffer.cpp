#include "Graphics/Renderers/LightUniformBuffer.h"
#include <GL\glew.h>
#include <corecrt_memcpy_s.h>
#include "Graphics/Shaders/Shader.h"

LightUniformBuffer::LightUniformBuffer()
{
	m_PLBuffer = 0;
	m_SLBuffer = 0;
	m_PointLightBlockSize = 0;
	m_SpotLightBlockSize = 0;
	m_pointLightBlockIndex = 0;
	m_splBlockIndex = 0;
	m_MaxPoints = 0;
	m_MaxSpots = 0;
}

void LightUniformBuffer::init(Shader& shader)
{
	shader.activate();
	glGenBuffers(1, &m_SLBuffer);
	glGenBuffers(1, &m_PLBuffer);
	int size = sizeof(LightData);
	size = sizeof(SpotLightData);
	//binding index for point light buffer
	m_pointLightBlockIndex = glGetUniformBlockIndex(shader.getShaderHandle(), "PointLights");
	glGetActiveUniformBlockiv(shader.getShaderHandle(), m_pointLightBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &m_PointLightBlockSize);
	m_MaxPoints = m_PointLightBlockSize / sizeof(LightData);
	glUniformBlockBinding(shader.getShaderHandle(), m_pointLightBlockIndex, 0);
	//binding index for spotlight buffer
	m_splBlockIndex = glGetUniformBlockIndex(shader.getShaderHandle(), "SpotLights");
	glGetActiveUniformBlockiv(shader.getShaderHandle(), m_splBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &m_SpotLightBlockSize);
	m_MaxSpots = m_SpotLightBlockSize / sizeof(SpotLightData);
	glUniformBlockBinding(shader.getShaderHandle(), m_splBlockIndex, 1);

	glBindBuffer(GL_UNIFORM_BUFFER, m_SLBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(SpotLightData)*m_MaxSpots, nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_UNIFORM_BUFFER, m_PLBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData)*m_MaxPoints, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glUseProgram(0);
}

void LightUniformBuffer::updatePointLights(int numLights, const LightData * data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_PLBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightData)*numLights, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightUniformBuffer::updateSpotLights(int numLights, const SpotLightData * data)
{
	glBindBuffer(GL_UNIFORM_BUFFER, m_SLBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(SpotLightData)*numLights, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightUniformBuffer::bindPointLights()
{
	glBindBufferBase(GL_UNIFORM_BUFFER, m_pointLightBlockIndex, m_PLBuffer);
}

void LightUniformBuffer::bindSpotLights()
{
	glBindBufferBase(GL_UNIFORM_BUFFER, m_splBlockIndex, m_SLBuffer);
}

LightUniformBuffer::~LightUniformBuffer()
{

	glDeleteBuffers(1, &m_SLBuffer);
	glDeleteBuffers(1, &m_PLBuffer);
}
