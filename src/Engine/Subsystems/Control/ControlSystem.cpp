#include "Engine/Subsystems/Control/ControlSystem.h"
#include "Engine/Controllers/Keyboard.h"
#include "Engine/Controllers/Mouse.h"


ControlSystem::ControlSystem()
{
	m_Controllers.resize((size_t)Controls::Num_Types);
	m_Messenger = nullptr;
}

std::shared_ptr<Controller> ControlSystem::getController(Controls c)
{
	return m_Controllers[(size_t)c];
}

std::shared_ptr<Mouse> ControlSystem::getMouse()
{
	return m_Mouse;
}

std::shared_ptr<Keyboard> ControlSystem::getKeyboard()
{
	return m_Keyboard;
}


ControlSystem::~ControlSystem()
{
	m_Controllers.clear();
}

void ControlSystem::handleEvent(SDL_Event & e)
{
	for (auto c : m_Controllers)
	{
		if (c != nullptr)
		{
			c->handleEvent(e);
		}
	}
}

void ControlSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	m_Mouse = std::make_shared<Mouse>();
	m_Controllers[0] = m_Mouse;
	m_Keyboard = std::make_shared<Keyboard>();
	m_Controllers[1] = m_Keyboard;
	m_Messenger = &messenger;
}

void ControlSystem::update(const float & deltaTimeS, EC_Game & game)
{
	for (auto c : m_Controllers)
	{
		if (c != nullptr)
		{
			c->update(*m_Messenger);
		}
	}
}


void ControlSystem::shutdown()
{
	m_Mouse->shutdown();
	m_Keyboard->shutdown();
}

KeyState ControlSystem::getKeyState(SDL_Scancode key)
{
	KeyState state = m_Keyboard->getKeyState(key);
	if (state != KeyState::Invalid)
	{
		return state;
	}
	state = m_Mouse->getKeyState(key);
	if (state != KeyState::Invalid)
	{
		return state;
	}
	return KeyState::None;
}
