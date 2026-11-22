#pragma once

#include "Messaging/ECXEvent.h"
#include <SDL.h>

class KeyEvent
{
public:
	KeyEvent();
	~KeyEvent();
	KeyEvent(SDL_Scancode key, bool pressed, bool held, float deltaTimeS = 0.0f);
	SDL_Scancode& getKey();
	const char* getKeyString();
	bool isPressed();
	bool isReleased();
	bool isHeld();
	// Per-frame delta time when this event was published; meaningful only for key_held (0.0 for
	// key_down/key_up). Lets an OnKeyHeld handler move a fixed distance per second instead of assuming a
	// frame time.
	float getDeltaTime();
private:
	SDL_Scancode m_key;
	bool m_held;
	bool m_pressed;
	float m_deltaTimeS;
};

