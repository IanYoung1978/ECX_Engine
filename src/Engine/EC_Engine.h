#pragma once
#include "Subsystems/EC_System.h"
#include "Subsystems/EC_SystemType.h"
#include "TaskManager/EC_SystemTask.h"
#include "TaskManager/EC_ThreadManager.h"
#include <memory>
#include <vector>
#include "Messaging/ECXMessenger.h"

class GameEntity;
class EC_Game;
class EC_Event;
class EC_EventQueue;

class EC_Engine:
	public ICommandListener
{
public:
	EC_Engine();
	void init(const std::string& config, EC_Game& game, ECXMessenger& messenger);
	// Inherited via ICommandListener
	void receive(ECXCommand& command) override;
	void addEntity(std::shared_ptr<GameEntity> e);
	void removeEntity(unsigned int entityID);
	void clearEntities();
	void pause();
	void resume();
	void start();
	void stop();
	~EC_Engine();
private:
	std::vector<std::shared_ptr<EC_System>> m_Systems;
	std::vector<std::shared_ptr<EC_SystemTask>> m_tasks;
	EC_Game* m_game;
	EC_ThreadManager m_threadpool;

};

