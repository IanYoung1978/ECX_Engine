#include "SDL_GL_Window.h"
#include <Windows.h>
#include <iostream>
using namespace std;
void APIENTRY openglCallbackFunction(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam) {

	cout << "---------------------opengl-callback-start------------" << endl;
	cout << "message: " << message << endl;
	cout << "type: ";
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		cout << "ERROR";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		cout << "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		cout << "UNDEFINED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		cout << "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		cout << "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_OTHER:
		cout << "OTHER";
		break;
	}
	cout << endl;

	cout << "id: " << id << endl;
	cout << "severity: ";
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:
		cout << "LOW";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		cout << "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		cout << "HIGH";
		break;
	}
	cout << endl;
	cout << "---------------------opengl-callback-end--------------" << endl;
}


SDL_GL_Window::SDL_GL_Window()
{
	m_Context = 0;
	m_Height = 0;
	m_Width = 0;
	m_Window = nullptr;
	m_FullScreen = false;
}


SDL_GL_Window::~SDL_GL_Window()
{
	if (m_Window != NULL)
		close();
}

bool SDL_GL_Window::init(WindowSettings & settings)
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0) // Initialize video
		return false;

	// Request an OpenGL 3.0 context.
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, settings.OGLMajor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, settings.OGLMinor);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  // double buffering on
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); // 8 bit alpha buffering
	if (settings.multisampling)
	{
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, settings.MSAALevel); // Turn on x4 multisampling anti-aliasing (MSAA)
	}


	m_Window = SDL_CreateWindow(
		settings.windowName.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		settings.width,
		settings.height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );

	if (m_Window == NULL)
		return false;
	if (settings.fullscreen)
		toggleFullscreen();
	m_Context = SDL_GL_CreateContext(m_Window);
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		close();
		return false;
	}

	m_Width = settings.width;
	m_Height = settings.height;
	// During init, enable debug output
//#ifndef _NDEBUG
//	glEnable(GL_DEBUG_OUTPUT);
//	glDebugMessageCallback((GLDEBUGPROC)openglCallbackFunction, 0);
//#endif
	return true;
}

void SDL_GL_Window::resize(int width, int height)
{
	SDL_SetWindowSize(m_Window, width, height);
}

void SDL_GL_Window::toggleFullscreen()
{
	if (!m_FullScreen)
	{
		m_FullScreen = true;
		SDL_SetWindowFullscreen(m_Window,
			SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else
	{
		m_FullScreen = false;
		SDL_SetWindowFullscreen(m_Window,
			0);
	}
}

void SDL_GL_Window::present()
{
	SDL_GL_SwapWindow(m_Window);
}

void SDL_GL_Window::close()
{
	SDL_GL_DeleteContext(m_Context);
	SDL_DestroyWindow(m_Window);
}

int SDL_GL_Window::getWidth()
{
	return m_Width;
}

int SDL_GL_Window::getHeight()
{
	return m_Height;
}
