#include "Graphics/Textures/PBR_TextureSet.h"
#include "Graphics/Textures/TextureManager.h"
#include "Graphics/Shaders/Shader.h"

PBR_TextureSet::PBR_TextureSet()
{
	m_Albedo = 0;
	m_Roughness = 0;
	m_Glow = 0;
	m_Normal = 0;
	m_Reflection = false;
	m_Parallax = 0;
	m_Metallic = 0;
	m_AO = 0;
	m_ParallaxBias = 0.001f;
	m_ParallaxScale = 0.02f;
}

PBR_TextureSet::~PBR_TextureSet()
{
}

void PBR_TextureSet::setTextureHandles(TextureManager& textureManager)
{
	m_Albedo = textureManager.getTexture(m_AlbedoName);
	m_Normal = textureManager.getTexture(m_NormalName);
	m_Parallax = textureManager.getTexture(m_ParallaxName);
	m_Roughness = textureManager.getTexture(m_RoughnessName);
	m_Metallic = textureManager.getTexture(m_MetallicName);
	m_AO = textureManager.getTexture(m_AOName);
	m_Glow = textureManager.getTexture(m_GlowName);
}

void PBR_TextureSet::setTexture(TextureID id, std::string& name)
{
	switch (id)
	{
	case (TextureID::Albedo):
	{
		m_AlbedoName = name;
	}
	break;
	case (TextureID::Normal):
	{
		m_NormalName = name;
	}
	break;
	case (TextureID::Roughness):
	{
		m_RoughnessName = name;
	}
	break;
	case (TextureID::Parallax):
	{
		m_ParallaxName = name;
	}
	break;
	case (TextureID::Diffuse):
	{
		// "Diffuse" isn't sent by PBR material XML parsing (which always uses "Albedo"),
		// but treat it as an Albedo alias rather than the previous dead/wrong fallthrough
		// into m_GlowName, in case anything ever does send it.
		m_AlbedoName = name;
	}
	break;
	case (TextureID::AO):
	{
		m_AOName = name;
	}
	break;
	case (TextureID::Metallic):
	{
		m_MetallicName = name;
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

MaterialType PBR_TextureSet::getMaterialType()
{
	return MaterialType::BPR;
}

void PBR_TextureSet::bindTextures(std::shared_ptr<Shader>& shader)
{
	shader->setUniform("hasMaterial", 1);
	shader->bindTexture("colourMap", 0, m_Albedo);
	shader->bindTexture("normalMap", 1, m_Normal);
	shader->bindTexture("heightMap", 2, m_Parallax);
	shader->bindTexture("glowMap", 3, m_Glow);
	shader->bindTexture("roughnessMap", 4, m_Roughness);
	shader->bindTexture("metalMap", 5, m_Metallic);
	shader->bindTexture("AOMap", 6, m_AO);
	shader->setUniform("parallaxScale", m_ParallaxScale);
	shader->setUniform("parallaxBias", m_ParallaxBias);
}
