#include "Shader.h"
#include <glm\gtc\type_ptr.hpp>
#include <GL\glew.h>
#include <fstream>
#include "Logging/ECX_Logging.h"

Shader::Shader()
{
	m_Shader = 0;
}

bool Shader::loadShader(const std::string & vertFile, const std::string & fragFile)
{
	GLint vLength = 0;
	GLint fLength = 0;
	char* vs = loadFile(vertFile, vLength);
	if ( vLength == 0 )
	{
		//error loading
		return false;
	}
	char* fs = loadFile(fragFile, fLength);
	if ( fLength == 0 )
	{
		//load error
		delete[] vs;
		return false;
	}

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint error = glGetError();
	const char* VV = vs;
	const char* FF = fs;

	glShaderSource(vert, 1, &VV, &vLength);
	error = glGetError();
	glShaderSource(frag, 1, &FF, &fLength);
	error = glGetError();
	int compiled = 0;
	glCompileShader(vert);
	error = glGetError();
	glGetShaderiv(vert, GL_COMPILE_STATUS, &compiled);
	error = glGetError();
	if (!compiled)
	{
		//compilation error
		outputShaderError(vert);
		return false;
	}
	glCompileShader(frag);
	error = glGetError();
	glGetShaderiv(frag, GL_COMPILE_STATUS, &compiled);
	error = glGetError();
	if (!compiled)
	{
		//compilation error
		outputShaderError(frag);
		glDeleteShader(vert);
		return false;
	}
	m_Shader = glCreateProgram();
	error = glGetError();
	glAttachShader(m_Shader, vert);
	error = glGetError();
	glAttachShader(m_Shader, frag);
	error = glGetError();
	bindAttributes();
	error = glGetError();
	glLinkProgram(m_Shader);
	error = glGetError();

	delete vs;
	delete fs;

	return true;
}

bool Shader::loadShader(char * vs, int vLength, char * fs, int fLength)
{
	if (vLength == 0)
	{
		//error loading
		return false;
	}
	if (fLength == 0)
	{
		//load error
		return false;
	}

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

	const char* VV = vs;
	const char* FF = fs;
	glShaderSource(vert, 1, &VV, &vLength);
	glShaderSource(frag, 1, &FF, &fLength);
	int compiled = 0;
	glCompileShader(vert);
	glGetShaderiv(vert, GL_COMPILE_STATUS, &compiled);
	outputShaderError(vert);
	if (!compiled)
	{
		//compilation error
		
		return false;
	}
	glCompileShader(frag);
	glGetShaderiv(frag, GL_COMPILE_STATUS, &compiled);
	outputShaderError(frag);
	if (!compiled)
	{
		//compilation error
		
		glDeleteShader(vert);
		return false;
	}
	m_Shader = glCreateProgram();
	glAttachShader(m_Shader, vert);
	glAttachShader(m_Shader, frag);
	bindAttributes();
	glLinkProgram(m_Shader);

	return true;
}

void Shader::setUniform(const std::string & uniformName, const glm::mat4 & matrix)
{
	int uniform = searchUniform(uniformName);
	glUniformMatrix4fv(uniform, 1, false, glm::value_ptr(matrix));
}

void Shader::setUniform(const std::string & uniformName, const glm::mat3 & matrix)
{
	int uniform = searchUniform(uniformName);
	glUniformMatrix3fv(uniform, 1, false, glm::value_ptr(matrix));
}

void Shader::setUniform(const std::string & uniformName, const glm::vec4 & vector)
{
	int uniform = searchUniform(uniformName);
	glUniform4fv(uniform, 1, glm::value_ptr(vector));
}

void Shader::setUniform(const std::string & uniformName, const glm::vec3 & vector)
{
	int uniform = searchUniform(uniformName);
	glUniform3fv(uniform, 1, glm::value_ptr(vector));
}

void Shader::setUniform(const std::string & uniformName, const glm::vec2 & vector)
{
	int uniform = searchUniform(uniformName);
	glUniform2fv(uniform, 1, glm::value_ptr(vector));
}

void Shader::setUniform(const std::string & uniformName, const float & value)
{
	int uniform = searchUniform(uniformName);
	glUniform1f(uniform, value);
}

void Shader::setUniform(const std::string & uniformName, const int & value)
{
	int uniform = searchUniform(uniformName);
	glUniform1i(uniform, value);
}

void Shader::setDirLight(const std::string & uniformName, const DirLightData & data)
{
	setUniform(uniformName + (".colour"), data.colour);
	setUniform(uniformName + (".intensity"), data.intensity);
	setUniform(uniformName + (".direction"), data.direction);
}

void Shader::setLight(const std::string & uniformName, const LightData & data)
{
	setUniform(uniformName + (".colour"), data.colour);
	setUniform(uniformName + (".intensity"), data.intensity);
	setUniform(uniformName + (".position"), data.position);
	setUniform(uniformName + (".attenuation"), data.attenuation);
}

void Shader::setSpotLight(const std::string & uniformName, const SpotLightData & data)
{
	setUniform(uniformName + (".colour"), data.colour);
	setUniform(uniformName + (".intensity"), data.intensity);
	setUniform(uniformName + (".direction"), data.direction);
	setUniform(uniformName + (".position"), data.position);
	setUniform(uniformName + (".cutoffAngle"), data.cutoffAngle);
	setUniform(uniformName + (".attenuation"), data.attenuation);
}

void Shader::bindTexture(int texTarget, unsigned int texHandle)
{

	switch (texTarget)
	{
	case 0: setUniform(("colourMap"), texTarget);
		break;
	case 1: setUniform(("specularMap"), texTarget);
		break;
	case 2: setUniform(("normalMap"), texTarget);
		break;
	case 3: setUniform(("heightMap"), texTarget);
		break;
	case 4: setUniform(("glowMap"), texTarget);
		break;
	case 5: setUniform(("smoothnessMap"), texTarget);
		break;
	case 6: setUniform(("metalMap"), texTarget);
		break;
	case 7: setUniform(("AOMap"), texTarget);
		break;
	default:
		break;
	}
	glActiveTexture(GL_TEXTURE0 + texTarget);
	glBindTexture(GL_TEXTURE_2D, texHandle);
}

void Shader::bindTexture(const std::string & uniformName, int texTarget, unsigned int texHandle)
{
	setUniform(uniformName, texTarget);
	glActiveTexture(GL_TEXTURE0+texTarget);
	glBindTexture(GL_TEXTURE_2D, texHandle);
}

bool Shader::bindAttribute(BufferType attribType, const std::string & attribName)
{
	glBindAttribLocation(m_Shader, (GLuint)attribType, attribName.c_str());
	GLint error = glGetError();
	if (error != 0)
		return false;
	return false;
}

void Shader::activate()
{
	glUseProgram(m_Shader);
}

int Shader::getShaderHandle()
{
	return m_Shader;
}

Shader::~Shader()
{
	glDeleteProgram(m_Shader);
}

char * Shader::loadFile(const std::string & filename, int & size)
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

void Shader::bindAttributes()
{
	glBindAttribLocation(m_Shader, (GLuint)BufferType::Vertex, "MSVertex");
	glBindAttribLocation(m_Shader, (GLuint)BufferType::Normal, "MSNormal");
	glBindAttribLocation(m_Shader, (GLuint)BufferType::TextureCoordinate, "MSTexCoord");
	glBindAttribLocation(m_Shader, (GLuint)BufferType::Tangent, "MSTangent");
	glBindAttribLocation(m_Shader, (GLuint)BufferType::BiTangent, "MSBiTangent");
	
}


unsigned int Shader::searchUniform(const std::string & uniformName)
{
	for (auto uniform : m_BoundUniforms)
	{
		if (uniform.first == uniformName)
		{
			return uniform.second;
		}
	}

	int uniform = glGetUniformLocation(m_Shader, uniformName.c_str());
	if (uniform > -1)
		m_BoundUniforms.push_back(std::pair<std::string, unsigned int>(uniformName, uniform));
	return uniform;
}

void Shader::outputShaderError(unsigned int shader)
{
	GLint maxLength = 0;
	GLint logLength = 0;
	char* logMessage;

	//find out how long message is
	if (!glIsShader(shader))
		glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
	else
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

	if (maxLength>0)//if log message has some content
	{
		logMessage = new GLchar[maxLength];
		if (!glIsShader(shader))
			glGetProgramInfoLog(shader, maxLength, &logLength, logMessage);
		else
			glGetShaderInfoLog(shader, maxLength, &logLength, logMessage);

		printf("Shader Info Log: %s\n", logMessage);
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Shader Info Log: " + std::string(logMessage), LOGGING::LogLevel::INFORMATION);
		delete[] logMessage;
	}
}




