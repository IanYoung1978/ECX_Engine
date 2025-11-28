#include "ADS_TextureSet.h"
#include "Shader.h"
#include "TextureManager.h"

ADS_TextureSet::ADS_TextureSet()
{
	m_Diffuse = 0;
	m_Specular = 0;
	m_Glow = 0;
	m_Normal = 0;
	m_Reflection = false;
	m_Parallax = 0;
}

void ADS_TextureSet::setTextureHandles(TextureManager& textureManager)
{
	m_Diffuse = textureManager.getTexture(m_DiffuseName);
	m_Normal = textureManager.getTexture(m_NormalName);
	m_Parallax = textureManager.getTexture(m_ParallaxName);
	m_Specular = textureManager.getTexture(m_SpecularName);
	m_Glow = textureManager.getTexture(m_GlowName);
}

void ADS_TextureSet::setTexture(TextureID id, std::string& name)
{
	switch (id)
	{
	case (TextureID::Diffuse):
	{
		m_DiffuseName = name;
	}
	break;
	case (TextureID::Normal):
	{
		m_NormalName = name;
	}
	break;
	case (TextureID::Specular):
	{
		m_SpecularName = name;
	}
	break;
	case (TextureID::Parallax):
	{
		m_ParallaxName = name;
	}
	break;
	case (TextureID::Glow):
	{
		m_GlowName = name;
	}
	break;
	default:
		break;
	}
}

MaterialType ADS_TextureSet::getMaterialType()
{
	return MaterialType::ADS;
}

void ADS_TextureSet::bindTextures(std::shared_ptr<Shader>& shader)
{
	shader->setUniform("hasMaterial", 1);
	shader->bindTexture(0, m_Diffuse);
	shader->bindTexture(1, m_Specular);
	shader->bindTexture(2, m_Normal);
	shader->bindTexture(3, m_Parallax);
	shader->bindTexture(4, m_Glow);
}

ADS_TextureSet::~ADS_TextureSet()
{
}
