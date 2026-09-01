#pragma once
#include "Graphics/Textures/TextureSet.h"
class PBR_TextureSet :
	public TextureSet
{
public:
	PBR_TextureSet();
	virtual ~PBR_TextureSet();
	// Inherited via TextureSet
	virtual void setTextureHandles(TextureManager& textureManager);
	virtual void setTexture(TextureID id, std::string& name);
	virtual MaterialType getMaterialType() override;
	virtual void bindTextures(std::shared_ptr<Shader>& shader) override;
	void setParallaxScale(float scale) { m_ParallaxScale = scale; }
	void setParallaxBias(float bias) { m_ParallaxBias = bias; }
private:
	unsigned int m_Albedo;
	unsigned int m_Smoothness;
	unsigned int m_Glow;
	unsigned int m_Normal;
	unsigned int m_Parallax;
	unsigned int m_AO;
	unsigned int m_Metallic;
	bool m_Reflection;
	float m_ParallaxScale;
	float m_ParallaxBias;
	std::string m_AlbedoName;
	std::string m_SmoothnessName;
	std::string m_GlowName;
	std::string m_NormalName;
	std::string m_ParallaxName;
	std::string m_AOName;
	std::string m_MetallicName;
};

