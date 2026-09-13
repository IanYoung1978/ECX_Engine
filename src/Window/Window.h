#pragma once
#include <string>
#include "WindowSettings.h"
class Window
{
public:
	Window();
	virtual bool init(WindowSettings& settings) = 0;
	// Requests an actual OS-level resize (e.g. from a debug/remote-control call) - the
	// concrete window's cached width/height are NOT updated by this call directly, only by
	// the SDL_WINDOWEVENT_SIZE_CHANGED event this triggers (see onResized) - that keeps a
	// single source of truth regardless of whether a resize came from this call or the user
	// dragging the window edge.
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

