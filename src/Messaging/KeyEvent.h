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
	// Real per-frame delta time at the moment this event was published - only meaningful
	// for key_held (0.0 for key_down/key_up, which are one-shot and have no "this frame's
	// duration" to speak of). Lets an OnKeyHeld handler move a fixed distance/second
	// instead of hardcoding an assumed frame time that silently drifts if the real
	// framerate isn't exactly what was assumed.
	float getDeltaTime();
private:
	SDL_Scancode m_key;
	bool m_held;
	bool m_pressed;
	float m_deltaTimeS;
};

