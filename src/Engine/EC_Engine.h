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
	void pause();
	void resume();
	// Runs every system's update() once, synchronously, on the calling thread -
	// independent of the threaded task loop and its pause flag. Used to bake a
	// correct initial transform/camera state (position, scale, view matrix)
	// before the very first render, since starting paused means the normal
	// per-tick loop may never run before that first frame is drawn.
	void stepOnce(float deltaTimeS);
	void start();
	void stop();
	~EC_Engine();
private:
	std::vector<std::shared_ptr<EC_System>> m_Systems;
	std::vector<std::shared_ptr<EC_SystemTask>> m_tasks;
	EC_Game* m_game;
	EC_ThreadManager m_threadpool;

};

