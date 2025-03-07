#include "ShaderManager.h"

#include <glm\gtc\type_ptr.hpp>
#include <GL\glew.h>
#include <fstream>
#include "BufferType.h"
#include "Logging/ECX_Logging.h"

ShaderManager::ShaderManager()
{
}

void ShaderManager::loadShader(const std::string & vert, const std::string & frag)
{
	auto dat = findShader(vert, frag);
	if (dat.flength == 0 || dat.vlength == 0)
	{
		char* vs = loadFile(vert, dat.vlength);
		if (dat.vlength == 0)
			return;
		char* fs = loadFile(frag, dat.flength);
		if (dat.vlength == 0)
		{
			delete vs;
			return;
		}
		dat.vs = vs;
		dat.fs = fs;
		m_ShaderData.emplace(std::make_pair(vert,frag), dat);
	}
}	

std::shared_ptr<Shader> ShaderManager::getShader(const std::string & vert, const std::string & frag)
{
	auto shader = m_Shaders.find(std::make_pair(vert, frag));
	if (shader != m_Shaders.end())
	{
		return shader->second;
	}
	return nullptr;
}

void ShaderManager::finaliseShaders()
{
	for (auto s : m_ShaderData)
	{
		if (getShader(s.second.vs, s.second.fs) == nullptr)
		{
			auto shader = std::make_shared<Shader>();

			if (shader->loadShader(s.second.vs, s.second.vlength, s.second.fs, s.second.flength))
			{
			
				m_Shaders.emplace(std::make_pair(s.first.first, s.first.second), shader);
				delete[] s.second.vs;
				s.second.vs = nullptr;
				delete[] s.second.fs;
				s.second.fs = nullptr;
				s.second.flength = 0;
				s.second.vlength = 0;
			}
			else
			{
				s.first.first;
				//throw/log error
				char m_error[256];
				sprintf_s(m_error, "Failed to load shader files to GPU: %s, or %s", s.first.first.c_str(), s.first.second.c_str());
				LOGGING::ECX_Logger::GetInstance()->LogMessage(m_error, LOGGING::LogLevel::CRITICAL);
			}
		}
	}
}

std::shared_ptr<Shader> ShaderManager::finaliseShader(const std::string& vert, const std::string& frag)
{
	// if this is being called then data should have been loaded from file
	// if find shader == false create shader
	// if find shader data == false log error
	// load shader from memory to gpu and store
	// return shader
	auto s = getShader(vert, frag);
	if (s == nullptr)
	{
		s = std::make_shared<Shader>();
		auto data = findShader(vert, frag);
		if (data.vlength == 0 || data.flength == 0)
		{
			// data not loaded: something is wrong: log error
			return s;
		}
		
		if (s->loadShader(data.vs, data.vlength, data.fs, data.flength))
		{
			m_Shaders.emplace(std::make_pair(vert, frag), s);
			delete[] data.vs;
			data.vs = nullptr;
			delete[] data.fs;
			data.fs = nullptr;
			data.flength = 0;
			data.vlength = 0;
		}
	}
	return s;
}


ShaderManager::~ShaderManager()
{
}

char * ShaderManager::loadFile(const std::string & filename, int & size)
{
	int filesize = 0;
	char* memBlock = nullptr;
	std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);
	if (file.is_open())
	{
		filesize = (int)file.tellg(); //get location of file point at end, which is the file size
		memBlock = new char[filesize];
		file.seekg(0, std::ios::beg);
		file.read(memBlock, filesize);
		file.close();
		size = (GLint)filesize;
	}
	return memBlock;
}

ShaderData ShaderManager::findShader(const std::string & vert, const std::string & frag)
{
	auto data = m_ShaderData.find(std::make_pair(vert, frag));
	if (data == m_ShaderData.end())
	{
		return ShaderData();
	}
	return data->second;
}
