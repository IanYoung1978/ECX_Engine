#include "TextureManager.h"
#include <GL\glew.h>
#include <algorithm>
#include <cstring>
#include "Logging/ECX_Logging.h"

TextureManager::TextureManager()
{

}

bool TextureManager::init()
{

	//Initialize PNG loading
	int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("SDL_image could not initialize!", LOGGING::LogLevel::CRITICAL);
		return false;
	}
	return true;
}

int TextureManager::loadTexture(const std::string & filename)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	GLuint texID = 0;
	texID = findTexture(filename);
	if (texID > 0)
		return (unsigned int)texID;
	SDL_Surface* surface = IMG_Load(filename.c_str());
	if (!surface)
	{
		printf_s("image load fail: %s\n", filename.c_str());
		LOGGING::ECX_Logger::GetInstance()->LogMessage("image load fail: " + filename, LOGGING::LogLevel::SEVERE);
		return -1;
	}
	//SDL_image loads pixels with incorrect origin for OpenGL, so we must flip the image, around the horizontal axis
	//flipYpixels(surface->format->BytesPerPixel, (char*)surface->pixels, surface->w, surface->h); //pitch is the true image width, including padding
	m_img_data.emplace(filename, surface);

	return 0;
}

unsigned int TextureManager::getTexture(const std::string & filename)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	GLuint texID = findTexture(filename);
	if (texID == 0)
		finalizeTexture(filename);
	return findTexture(filename);
}

void TextureManager::finalizeTextures()
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	for (auto img : m_img_data)
	{
		finalizeTexture(img.first, img.second);
	}
}

void TextureManager::finalizeTexture(const std::string & filename)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	auto img = m_img_data.find(filename);
	if (img != m_img_data.end()) {
		finalizeTexture(img->first, img->second);
	}
}


TextureManager::~TextureManager()
{
	IMG_Quit();
}

unsigned int TextureManager::findTexture(const std::string& filename)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	auto it = m_Textures.find(filename);
	if (it == m_Textures.end())
		return 0;
	return it->second;
}

void TextureManager::flipYpixels(int BPP, char* pixels, int width, int height)
{
	//move down through each row and swap with the corresponding opposite row
	// 111111111111111111111111111111111   <-- Copy to buffer(1)
	// 222222222222222222222222222222222   
	// 333333333333333333333333333333333   <-- Stop after this
	// 444444444444444444444444444444444
	// 555555555555555555555555555555555
	// 666666666666666666666666666666666   <-- Copy to 1(2), then copy buffer to here(3)

	//create temp buffer
	char* copy = new char[width*BPP];

	for (int i = 0; i < height / 2; i++) //height/2 is always rounded down, so this will account for odd height
	{
		//copy to buffer
		int bytes_per_row = width*BPP;
		std::memcpy(copy, &pixels[bytes_per_row*i], bytes_per_row);
		//copy from height - i to original location
		std::memcpy(&pixels[bytes_per_row*i],  & pixels[bytes_per_row*(height - i)], bytes_per_row);
		//copy buffer to second location
		std::memcpy(&pixels[bytes_per_row*(height - i)], copy, bytes_per_row);
	}
	delete[] copy;
}

unsigned int TextureManager::finalizeTexture(const std::string & fname, SDL_Surface *surface)
{
	std::lock_guard<std::recursive_mutex> lock(m_Mutex);
	GLuint texID = 0;
	texID = findTexture(fname);
	if (texID == 0)
	{
		int BPP = surface->format->BytesPerPixel;
		int mode = GL_RGB;
		if (BPP == 4) {
			mode = GL_RGBA;
		}
		//generate OpenGL texture
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);
		glTexImage2D(GL_TEXTURE_2D, 0, mode, surface->w, surface->h, 0, mode, GL_UNSIGNED_BYTE, surface->pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		m_Textures.insert(std::pair<std::string, unsigned int>(fname, texID));
	}
	return texID;
}
