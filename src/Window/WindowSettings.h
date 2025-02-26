#pragma once
class WindowSettings
{
public:
	int width, height;
	bool fullscreen, borderless;
	std::string windowName;
	int OGLMinor, OGLMajor;
	int DXVersion;
	bool multisampling;
	int MSAALevel;
};