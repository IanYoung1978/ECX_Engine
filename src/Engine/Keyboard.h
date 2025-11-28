#pragma once
#include "Controller.h"
#include <vector>
#include <string>

class Keyboard :
	public Controller
{
public:
	Keyboard();
	virtual ~Keyboard();
	// Inherited via Controller
	virtual void update(ECXMessenger& messenger) override;
	virtual void handleEvent(SDL_Event& e) override;
	virtual bool keyPressed(SDL_Scancode key) override;
	virtual bool keyHeld(SDL_Scancode key) override;
	virtual bool keyReleased(SDL_Scancode key) override;
	virtual ControllerType getControllerType() override;
	virtual bool changed() override;
	virtual KeyState getKeyState(SDL_Scancode key) override;
private:
	bool m_Changed;
	bool m_PressedBuffer[256];
	bool m_HeldBuffer[256];
	std::vector<std::string> m_keyMap;
};

