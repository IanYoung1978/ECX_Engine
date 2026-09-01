#pragma once
#include <memory>
#include <string>
class Shader;
class TextureManager;
enum class MaterialType
{
	ADS,
	BPR,
	Num_Types
};

enum class TextureID
{
	Diffuse,
	Specular,
	Normal,
	Parallax,
	Glow,
	Albedo,
	Metallic,
	Smoothness,
	AO,
	NumIDs
};

class TextureSet
{
public:
	TextureSet();
	virtual void setTextureHandles(TextureManager& textureManager) = 0;
	virtual MaterialType getMaterialType()=0;
	virtual void setTexture(TextureID id, std::string& name)=0;
	virtual void bindTextures(std::shared_ptr<Shader>& shader)=0;
	virtual ~TextureSet();
};

