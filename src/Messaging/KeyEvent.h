#pragma once

#include "Messaging/ECXEvent.h"
#include <SDL.h>

class KeyEvent
{
public:
	KeyEvent();
	~KeyEvent();
	KeyEvent(SDL_Scancode key, bool pressed, bool held);
	SDL_Scancode& getKey();
	const char* getKeyString();
	bool isPressed();
	bool isReleased();
	bool isHeld();
private:
	SDL_Scancode m_key;
	bool m_held;
	bool m_pressed;
};

