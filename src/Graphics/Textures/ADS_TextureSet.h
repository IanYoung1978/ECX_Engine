#pragma once
#include "Graphics/Textures/TextureSet.h"
class ADS_TextureSet :
	public TextureSet
{
public:
	ADS_TextureSet();
	// Inherited via TextureSet
	virtual void setTextureHandles(TextureManager& textureManager);
	virtual void setTexture(TextureID id, std::string& name);
	virtual MaterialType getMaterialType() override;
	virtual void bindTextures(std::shared_ptr<Shader>& shader) override;
	virtual ~ADS_TextureSet();
private:
private:
	unsigned int m_Diffuse;
	unsigned int m_Specular;
	unsigned int m_Glow;
	unsigned int m_Normal;
	unsigned int m_Parallax;
	bool m_Reflection;
	std::string m_DiffuseName;
	std::string m_SpecularName;
	std::string m_GlowName;
	std::string m_NormalName;
	std::string m_ParallaxName;
};

