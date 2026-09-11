#pragma once
class WindowSettings
{
public:
	int width = 0, height = 0;
	bool fullscreen = false, borderless = false;
	std::string windowName;
	int OGLMinor = 0, OGLMajor = 0;
	int DXVersion = 0;
	// false/1 unless an <AA> tag is present in window.xml (see XML.h's Window parser) - see
	// window.xml's own comment for why this engine deliberately doesn't request window-level
	// MSAA. Previously uninitialized when no <AA> tag was present, an indeterminate-value
	// bug independent of (but easy to conflate with) that same MSAA/blit issue.
	bool multisampling = false;
	int MSAALevel = 1;
};