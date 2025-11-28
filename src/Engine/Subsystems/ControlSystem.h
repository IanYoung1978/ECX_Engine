#pragma once
#include <memory>
#include "Engine/Controller.h"
#include "EC_System.h"
#include <vector>
#include "Engine/Mouse.h"
#include "Engine/Keyboard.h"

class GameController;
class Touch;
class EC_Game;
enum class Controls
{
	Mouse,
	Keyboard,
	Controller,
	Touch,
	Num_Types
};

class ControlSystem
	:EC_System
{
public:
	ControlSystem();
	std::shared_ptr<Controller> getController(Controls c);
	std::shared_ptr<Mouse> getMouse();
	std::shared_ptr<Keyboard> getKeyboard();
	~ControlSystem();
	virtual void handleEvent(SDL_Event & e);
	// Inherited via EC_System
	virtual void init(ECXMessenger& messenger, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game & game) override;
	void shutdown();
	KeyState getKeyState(SDL_Scancode key);
private:
	std::vector<std::shared_ptr<Controller>> m_Controllers;
	std::shared_ptr<Mouse> m_Mouse;
	std::shared_ptr<Keyboard> m_Keyboard;
	ECXMessenger* m_Messenger;
};

