#pragma once
#include <memory>
#include <vector>
#include "Engine/GameMode.h"
#include "Window/Window.h"
#include "Engine/Timer.h"
#include "Engine/Controller.h"
#include "Engine/Subsystems/ControlSystem.h"
#include "Engine/EC_Engine.h"
#include "Messaging/ECXMessenger.h"
#include <mutex>
#include "TaskManager/EC_ThreadManager.h"

enum class Game_Error
{
	NO_ERROR,
	CONFIG_ERROR,
	WINDOW_ERROR,
	NUM_ERRORS
};

class EC_Game:
	public ICommandListener
{
public:
	EC_Game();
	Game_Error init(const std::string& configurationFilename);
	Game_Error run();
	void addEntity(std::shared_ptr<GameEntity> e);
	void removeEntity(unsigned int gameEntityID);
	EntityID  getEntityByName(const std::string& eName);
	void clearEntities();
	void shutDown();
	void update(const float& deltaTimeS);
	void changeMode(Game_Mode mode);
	KeyState getKeyState(SDL_Scancode key);
	~EC_Game();
private:
	friend class EC_GameMode;
	ECXMessenger m_Messenger;
	void startGame();
	std::shared_ptr<Window> getWindow();
	std::shared_ptr<ControlSystem> getControls();
	size_t m_CurrentMode;
	std::vector<std::unique_ptr<EC_GameMode>> m_Modes;
	std::shared_ptr<Window> m_Window;
	std::unique_ptr<Timer> m_Timer;
	std::shared_ptr<ControlSystem> m_Controls;
	bool m_Running;
	float m_Accumulator;
	EC_ThreadManager m_threadmanager;
	std::mutex m_lock;

	// Inherited via ICommandListener
	void receive(ECXCommand& command) override;
};

