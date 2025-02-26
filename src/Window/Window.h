#pragma once
#include <string>
#include "WindowSettings.h"
class Window
{
public:
	Window();
	virtual bool init(WindowSettings& settings) = 0;
	virtual void resize(int width, int height) = 0;
	virtual void toggleFullscreen() = 0;
	virtual void present() = 0;
	virtual void close() = 0;
	virtual int getWidth() = 0;
	virtual int getHeight() = 0;
	virtual ~Window();
};

