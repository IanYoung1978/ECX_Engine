#pragma once
#include <string>
#include "WindowSettings.h"
class Window
{
public:
	Window();
	virtual bool init(WindowSettings& settings) = 0;
	// Requests an OS-level resize. The cached width/height update only from the
	// SDL_WINDOWEVENT_SIZE_CHANGED this triggers (see onResized), so there's one source of truth whether
	// the resize came from here or a user drag.
	virtual void resize(int width, int height) = 0;
	// Called from the main event loop when the OS reports the window's size actually
	// changed (user drag, or as a side effect of resize() above) - updates cached
	// width/height so getWidth()/getHeight() reflect the new size on the very next call.
	virtual void onResized(int width, int height) = 0;
	virtual void toggleFullscreen() = 0;
	virtual void maximize() = 0;
	virtual void minimize() = 0;
	virtual void present() = 0;
	virtual void close() = 0;
	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	virtual ~Window();
};

