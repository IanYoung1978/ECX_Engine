#include "KeyEvent.h"



KeyEvent::KeyEvent():m_pressed(false), m_held(false), m_key(SDL_SCANCODE_UNKNOWN)
{
}


KeyEvent::~KeyEvent()
{
}

KeyEvent::KeyEvent(SDL_Scancode key, bool pressed, bool held)
{
	m_key = key;
	m_pressed = pressed;
	m_held = held;
}

SDL_Scancode & KeyEvent::getKey()
{
	return m_key;
}

const char * KeyEvent::getKeyString()
{
	return SDL_GetScancodeName(m_key);
}

bool KeyEvent::isPressed()
{
	return m_pressed && !m_held;
}

bool KeyEvent::isReleased()
{
	return m_held && !m_pressed;
}

bool KeyEvent::isHeld()
{
	return m_held && m_pressed;
}

