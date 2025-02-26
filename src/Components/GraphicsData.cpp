#include "GraphicsData.h"
#include "Graphics/ShaderManager.h"
#include "Entity/EntityFactory.h"

GraphicsData::GraphicsData()
{
	m_loaded = false;
}


GraphicsData::~GraphicsData()
{
}

ComponentType GraphicsData::getComponentType()
{
	return ComponentType::GraphicsData;
}

void GraphicsData::setModel(std::shared_ptr<ObjModel> model)
{
	m_Model = model;
	notifyObservers();
}

void GraphicsData::setModelName(const std::string & fname)
{
	m_model_name = fname;
	notifyObservers();
}

const std::string & GraphicsData::getModelName()
{
	return m_model_name;
}

void GraphicsData::setShaderNames(const std::string & vert, const std::string & frag)
{
	m_vertname = vert;
	m_fragname = frag;
	notifyObservers();
}

const std::string & GraphicsData::getFragShaderName()
{
	return m_fragname;
}

const std::string & GraphicsData::getVertexShaderName()
{
	return m_vertname;
}

void GraphicsData::setTextures(std::shared_ptr<TextureSet> textures)
{
	m_Textures = textures;
	notifyObservers();
}

void GraphicsData::setShader(std::shared_ptr<Shader> shader)
{
	m_Shader = shader;
	notifyObservers();
}

void GraphicsData::setColour(glm::vec4 colour)
{
	m_Colour = colour;
	notifyObservers();
}

std::shared_ptr<Shader> GraphicsData::getShader()
{
	return m_Shader;
}

std::shared_ptr<ObjModel> GraphicsData::getModel()
{
	return m_Model;
}

std::shared_ptr<TextureSet> GraphicsData::getTextures()
{
	return m_Textures;
}

glm::vec4 GraphicsData::getColour()
{
	return m_Colour;
}

void GraphicsData::Finalize()
{
	if (!m_loaded)
	{
		EntityFactory factory;
		m_Model = factory.s_MeshManager.finaliseModel(m_model_name);
		m_Shader = factory.s_ShaderManager.finaliseShader(m_vertname, m_fragname);
		if (m_Textures != nullptr)
		{
			m_Textures->setTextureHandles(factory.s_TexManager);
		}
		m_loaded = true;
	}
}

