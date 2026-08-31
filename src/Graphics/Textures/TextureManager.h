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
	int loadTexture(const std::string& filename);
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
	std::recursive_mutex m_Mutex;
};

