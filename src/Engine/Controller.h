#pragma once
#include <SDL.h>

#include "Messaging/ECXMessenger.h"

enum ControllerType
{
	MOUSE,
	KEYBOARD
};

enum class KeyState
{
	None,
	Pressed,
	Held,
	Released,
	Invalid
};

class EC_Game;
class Controller
{
public:
	Controller();
	virtual void update(ECXMessenger& messenger) = 0;
	virtual void handleEvent(SDL_Event& e) = 0;
	virtual bool keyPressed(SDL_Scancode key) = 0;
	virtual bool keyHeld(SDL_Scancode key) = 0;
	virtual bool keyReleased(SDL_Scancode key) = 0;
	virtual ControllerType getControllerType() = 0;
	virtual bool changed() = 0;
	virtual KeyState getKeyState(SDL_Scancode key) = 0;
	void shutdown();
	virtual ~Controller();
protected:
	bool m_running;
};

