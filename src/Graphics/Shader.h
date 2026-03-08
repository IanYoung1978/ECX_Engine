#pragma once
#include <string>
#include <glm\glm.hpp>
#include <vector>
#include <tuple>
#include "LightData.h"
#include "BufferType.h"
class Shader
{
public:
	Shader();
	bool loadShader(const std::string& vertFile, const std::string& fragFile);
	bool loadShader(char* vs, int vLength, char* fs, int fLength);
	void setUniform(const std::string& uniformName, const glm::mat4& matrix);
	void setUniform(const std::string& uniformName, const glm::mat3& matrix);
	void setUniform(const std::string& uniformName, const glm::vec4& vector);
	void setUniform(const std::string& uniformName, const glm::vec3& vector);
	void setUniform(const std::string& uniformName, const glm::vec2& vector);
	void setUniform(const std::string& uniformName, const float& value);
	void setUniform(const std::string& uniformName, const int& value);
	void setDirLight(const std::string& uniformName, const DirLightData& data);
	void setLight(const std::string& uniformName, const LightData& data);
	void setSpotLight(const std::string& uniformName, const SpotLightData& data);
	void bindTexture(int texTarget, unsigned int texHandle);
	void bindTexture(const std::string& uniformName, int texTarget, unsigned int texHandle);
	void bindCubemap(const std::string& uniformName, int texUnit, unsigned int texHandle);
	bool bindAttribute(BufferType attribType, const std::string& attribName);
	void activate();
	int getShaderHandle();
	~Shader();
private:
	char* loadFile(const std::string& filename, int& size);
	void bindAttributes();
	unsigned int searchUniform(const std::string& uniformName);
	void outputShaderError(unsigned int shader);
	int m_Shader;
	std::vector<std::pair<std::string, int>> m_BoundUniforms;
	int m_currentTexCount;
};

