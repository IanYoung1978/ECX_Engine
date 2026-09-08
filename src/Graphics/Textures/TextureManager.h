#pragma once
#include <string>
#include <string.h>
#include <map>
#include <mutex>
#include <SDL_image.h>

class TextureManager
{
public:
	TextureManager();
	bool init();
	// isSRGB: pass true only for colour/albedo data - it selects an sRGB-aware GL internal
	// format so the GPU decodes sRGB->linear automatically on sampling. Normal/height/
	// glow/smoothness/metallic/AO maps are not colour data and must stay false (the
	// default) - decoding them through the sRGB curve would corrupt their values.
	int loadTexture(const std::string& filename, bool isSRGB = false);
	unsigned int getTexture(const std::string& filename);
	void finalizeTextures();
	void finalizeTexture(const std::string& filename);
	~TextureManager();
private:
	unsigned int findTexture(const std::string& filename);
	void flipYpixels(int BPP, char* pixels, int width, int height);
	unsigned int finalizeTexture(const std::string& fname, SDL_Surface* surface);
	std::map<std::string, SDL_Surface*> m_img_data;
	std::map<std::string, unsigned int> m_Textures;
	std::map<std::string, bool> m_IsSRGB;
	std::recursive_mutex m_Mutex;
};

