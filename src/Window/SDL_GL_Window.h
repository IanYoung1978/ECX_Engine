#pragma once
#include "Window.h"
#include <SDL.h>
#include <GL\glew.h>
#include <glm\glm.hpp>
class SDL_GL_Window :
	public Window
{
public:
	SDL_GL_Window();
	virtual ~SDL_GL_Window();

	// Inherited via Window
	virtual bool init(WindowSettings & settings) override;
	virtual void resize(int width, int height) override;
	virtual void toggleFullscreen() override;
	virtual void present() override;
	virtual void close() override;
	virtual int getWidth() override;
	virtual int getHeight() override;
private:

	SDL_GLContext m_Context;
	SDL_Window* m_Window;
	bool m_FullScreen;
	int m_Width, m_Height;
};

