#pragma once
class WindowSettings
{
public:
	int width = 0, height = 0;
	bool fullscreen = false, borderless = false;
	std::string windowName;
	int OGLMinor = 0, OGLMajor = 0;
	int DXVersion = 0;
	// false/1 unless an <AA> tag is present in window.xml; see window.xml's comment for why window-level
	// MSAA isn't requested.
	bool multisampling = false;
	int MSAALevel = 1;
	// false unless <Resizable> is present in window.xml (see XML.h's Window parser) - a
	// game that doesn't ask for a resizable window shouldn't get one, some may deliberately
	// want a fixed layout.
	bool resizable = false;
};