#include "Engine/Controllers/Keyboard.h"
#include "Game.h"
#include "Messaging/KeyEvent.h"

Keyboard::Keyboard() :m_Changed(false)
{
	std::fill_n(m_PressedBuffer, 256, false);
	std::fill_n(m_HeldBuffer, 256, false);
	for (size_t i = 0; i < 256; i++)
	{
		m_PressedBuffer[i] = false;
		m_HeldBuffer[i] = false;
	}
}


Keyboard::~Keyboard()
{

}

void Keyboard::update(ECXMessenger& messenger)
{
	if (m_running == true)
	{
		for (size_t i = 0; i < 256; i++)
		{
			
			if (m_PressedBuffer[i] && m_HeldBuffer[i])
			{
				ECXEvent Event;
				Event.type = ECXEventType::key_held;
				Event.args[0] = KeyEvent((SDL_Scancode)i, true, true);
				messenger.publish(Event);
			}
			else if (!m_PressedBuffer[i] && m_HeldBuffer[i])
			{
				ECXEvent Event;
				Event.type = ECXEventType::key_up;
				Event.args[0] = KeyEvent((SDL_Scancode)i, false, true);
				messenger.publish(Event);
			}
			else if (m_PressedBuffer[i] && !m_HeldBuffer[i])
			{
				ECXEvent Event;
				Event.type = ECXEventType::key_down;
				Event.args[0] = KeyEvent((SDL_Scancode)i, true, false);
				messenger.publish(Event);
			}

			if (m_PressedBuffer[i])
				m_HeldBuffer[i] = true;
			else
				m_HeldBuffer[i] = false;
		}
	}
}

void Keyboard::handleEvent(SDL_Event & e)
{
	if (e.type == SDL_KEYDOWN)
		m_PressedBuffer[e.key.keysym.scancode] = true;
	else if (e.type == SDL_KEYUP)
		m_PressedBuffer[e.key.keysym.scancode] = false;
}


bool Keyboard::keyPressed(SDL_Scancode key)
{
	return (m_PressedBuffer[key] && !m_HeldBuffer[key]);
}

bool Keyboard::keyHeld(SDL_Scancode key)
{
	return (m_PressedBuffer[key] && m_HeldBuffer[key]);
}

bool Keyboard::keyReleased(SDL_Scancode key)
{
	return (!m_PressedBuffer[key] && m_HeldBuffer[key]);
}

ControllerType Keyboard::getControllerType()
{
	return KEYBOARD;
}


bool Keyboard::changed()
{
	return m_Changed;
}

KeyState Keyboard::getKeyState(SDL_Scancode key)
{
	if ((unsigned int)key >= 256)
		return KeyState::Invalid;
	if (keyHeld(key))
		return KeyState::Held;
	if (keyPressed(key))
		return KeyState::Pressed;
	if (keyReleased(key))
		return KeyState::Released;
	return KeyState::None;
}

