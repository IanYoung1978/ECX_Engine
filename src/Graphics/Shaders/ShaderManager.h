#pragma once
#include "Graphics/Shaders/Shader.h"
#include <memory>
#include <map>
#include <tuple>
#include <mutex>

struct ShaderData
{
	char* vs;
	int vlength;
	char* fs;
	int flength;
	ShaderData() {
		vs = nullptr;
		fs = nullptr;
		vlength = 0;
		flength = 0;
	}
};
class ShaderManager
{
public:
	ShaderManager();
	void loadShader(const std::string& vert, const std::string& frag);
	std::shared_ptr<Shader> getShader(const std::string& vert, const std::string& frag);
	void finaliseShaders();
	std::shared_ptr<Shader> finaliseShader(const std::string& vert, const std::string& frag);
	~ShaderManager();
private:
	char* loadFile(const std::string& filename, int& size);
	ShaderData findShader(const std::string& vert, const std::string& frag);
	std::map<std::pair<const std::string, const std::string>, ShaderData> m_ShaderData;
	std::map<std::pair<const std::string, const std::string>, std::shared_ptr<Shader>> m_Shaders;
	std::recursive_mutex m_Mutex;

};

