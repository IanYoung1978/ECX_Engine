#include "Mouse.h"
#include "Game.h"
#include "Messaging/MouseEvent.h"


Mouse::Mouse(): m_Changed(false)
{
	std::fill_n(m_PressedBuffer, 5, false);
	std::fill_n(m_HeldBuffer, 5, false);

	m_MouseMotion = glm::ivec2(0, 0);
	m_LastMouseMotion = glm::ivec2(0, 0);
	m_mouse_position = glm::ivec2(0, 0);
	m_FPS_Mode = true;
	SDL_SetRelativeMouseMode(SDL_TRUE);
}


Mouse::~Mouse()
{
}

void Mouse::update(ECXMessenger& messenger)
{
	if (m_running == true)
	{
		for (int i = 0; i < Num_Mouse_Buttons; i++)
		{
			// Edge-detect against last frame's held state BEFORE syncing it below - matches
			// Keyboard::update()'s ordering. Syncing first (the old code) meant m_HeldBuffer
			// always equalled m_PressedBuffer by the time it was checked, so "held"/"up" were
			// unreachable and any downstream keyPressed(MouseButton) edge check (e.g. UI click
			// hit-testing) could never see a press.
			if (m_PressedBuffer[i] && m_HeldBuffer[i])
			{
				MouseEvent m(static_cast<MouseButton>(i), m_PressedBuffer[i], m_HeldBuffer[i], m_mouse_position.x, m_mouse_position.y);
				ECXEvent Event;
				Event.type = ECXEventType::mouse_held;
				Event.args[0] = m;
				messenger.publish(Event);
			}
			else if (!m_PressedBuffer[i] && m_HeldBuffer[i])
			{
				MouseEvent m(static_cast<MouseButton>(i), m_PressedBuffer[i], m_HeldBuffer[i], m_mouse_position.x, m_mouse_position.y);
				ECXEvent Event;
				Event.type = ECXEventType::mouse_up;
				Event.args[0] = m;
				messenger.publish(Event);
			}
			else if (m_PressedBuffer[i] && !m_HeldBuffer[i])
			{
				MouseEvent m(static_cast<MouseButton>(i), m_PressedBuffer[i], m_HeldBuffer[i], m_mouse_position.x, m_mouse_position.y);
				ECXEvent Event;
				Event.type = ECXEventType::mouse_down;
				Event.args[0] = m;
				messenger.publish(Event);
			}

			m_HeldBuffer[i] = m_PressedBuffer[i];
		}

		if (m_MouseMotion.x != 0 || m_MouseMotion.y != 0)
		{
			MouseEvent m(MouseButton::Motion, false, false, m_mouse_position.x, m_mouse_position.y, m_MouseMotion.x, m_MouseMotion.y);
			ECXEvent Event;
			Event.args[0] = m;
			Event.type = ECXEventType::mouse_move;
			messenger.publish(Event);
		}
		m_MouseMotion = glm::ivec2(0, 0);
	}
}

void Mouse::handleEvent(SDL_Event & e)
{
	if (e.type == SDL_MOUSEBUTTONDOWN)
	{
		switch (e.button.button)
		{
		case SDL_BUTTON_LEFT: m_PressedBuffer[static_cast<int>(MouseButton::LMB)] = true;
			break;
		case SDL_BUTTON_RIGHT: m_PressedBuffer[static_cast<int>(MouseButton::RMB)] = true;
			break;
		case SDL_BUTTON_MIDDLE: m_PressedBuffer[static_cast<int>(MouseButton::Middle)] = true;
			break;
		case SDL_BUTTON_X1: m_PressedBuffer[static_cast<int>(MouseButton::MB4)] = true;
			break;
		case SDL_BUTTON_X2: m_PressedBuffer[static_cast<int>(MouseButton::MB5)] = true;
			break;
		default:
			break;
		}
		SDL_GetMouseState(&m_mouse_position.x, &m_mouse_position.y);
	}
	else if (e.type == SDL_MOUSEBUTTONUP)
	{
		switch (e.button.button)
		{
		case SDL_BUTTON_LEFT: m_PressedBuffer[static_cast<int>(MouseButton::LMB)] = false;
			break;
		case SDL_BUTTON_RIGHT: m_PressedBuffer[static_cast<int>(MouseButton::RMB)] = false;
			break;
		case SDL_BUTTON_MIDDLE: m_PressedBuffer[static_cast<int>(MouseButton::Middle)] = false;
			break;
		case SDL_BUTTON_X1: m_PressedBuffer[static_cast<int>(MouseButton::MB4)] = false;
			break;
		case SDL_BUTTON_X2: m_PressedBuffer[static_cast<int>(MouseButton::MB5)] = false;
			break;
		default:
			break;
		}
		SDL_GetMouseState(&m_mouse_position.x, &m_mouse_position.y);
	}
	else if (e.type == SDL_MOUSEMOTION)
	{
		m_MouseMotion.x = e.motion.xrel;
		m_MouseMotion.y = e.motion.yrel;
		m_mouse_position.x = e.motion.x;
		m_mouse_position.y = e.motion.y;
	}
}

bool Mouse::keyPressed(SDL_Scancode key)
{
	return false;
}

bool Mouse::keyHeld(SDL_Scancode key)
{
	return false;
}

bool Mouse::keyReleased(SDL_Scancode key)
{
	return false;
}

bool Mouse::keyPressed(MouseButton key)
{
	return (!m_HeldBuffer[(int)key] && m_PressedBuffer[(int)key]);
}

bool Mouse::keyHeld(MouseButton key)
{
	return (m_HeldBuffer[(int)key] && m_PressedBuffer[(int) key]);
}

bool Mouse::keyReleased(MouseButton key)
{
	return (m_HeldBuffer[(int)key] && !m_PressedBuffer[(int)key]);
}

void Mouse::setFPSMode(bool toggle)
{
	if(toggle)
		SDL_SetRelativeMouseMode(SDL_TRUE);
	else
		SDL_SetRelativeMouseMode(SDL_FALSE);
	m_FPS_Mode = toggle;
}

ControllerType Mouse::getControllerType()
{
	return MOUSE;
}

glm::ivec2 Mouse::getMouseMovement()
{
	return m_MouseMotion;
}

bool Mouse::changed()
{
	return m_Changed;
}

KeyState Mouse::getKeyState(SDL_Scancode key)
{
	if (key != SDL_BUTTON_LEFT && key != SDL_BUTTON_RIGHT && key != SDL_BUTTON_MIDDLE
		&& key != SDL_BUTTON_X1 && key != SDL_BUTTON_X2)
	return KeyState::Invalid;
	if (keyPressed(key))
		return KeyState::Pressed;
	if (keyHeld(key))
		return KeyState::Held;
	if (keyReleased(key))
		return KeyState::Released;
	return KeyState::None;
}
