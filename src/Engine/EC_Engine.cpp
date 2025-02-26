#include "EC_Engine.h"
#include "Entity/GameEntity.h"
#include "Subsystems/EC_SpatialSystem.h"
#include "TaskManager/EC_PhysicsThreadTask.h"
#include "Subsystems/EC_CameraSystem.h"
#include "Subsystems/EC_TransformSystem.h"
#include "Subsystems/EC_ScriptingSystem.h"
#include "TaskManager/EC_ScriptingTask.h"

EC_Engine::EC_Engine()
{
	m_game = nullptr;
	m_Systems.resize((size_t)EC_SystemType::Num_Systems, nullptr);
}

void EC_Engine::init(const std::string& config, EC_Game& game, ECXMessenger& messenger)
{
	m_game = &game;
	m_Systems[(size_t)EC_SystemType::Spatial] = std::make_shared<EC_SpatialSystem>();
	m_Systems[(size_t)EC_SystemType::Transform] = std::make_shared<EC_TransformSystem>();
	m_Systems[(size_t)EC_SystemType::Camera] = std::make_shared<EC_CameraSystem>();
	m_Systems[(size_t)EC_SystemType::Scripting] = std::make_shared<EC_ScriptingSystem>();
	for (auto s : m_Systems)
	{
		if (s != nullptr)
		{
			s->init(messenger, game);
		}
	}
	auto task = std::make_shared<EC_PhysicsThreadTask>();
	task->addGameRef(*m_game);
	task->addSystem(m_Systems[(size_t)EC_SystemType::Spatial]);
	task->addSystem(m_Systems[(size_t)EC_SystemType::Transform]);
	task->addSystem(m_Systems[(size_t)EC_SystemType::Camera]);
	// add additional systems (collision, physics)
	task->setTimeStep(1.0f / 60);
	m_tasks.push_back(task);
}


void EC_Engine::start()
{
	for (auto task : m_tasks)
	{
		task->start();

		m_threadpool.addTask(task);
	}
	// add other thread tasks
	// start engine
	m_threadpool.executeTasks();
}

void EC_Engine::stop()
{
	for (auto t : m_tasks)
	{
		t->stop();
	}
}

void EC_Engine::addEntity(std::shared_ptr<GameEntity> e)
{
	for (size_t i = 0; i < m_Systems.size(); i++)
	{
		if (m_Systems[i] != nullptr)
		{
			m_Systems[i]->addEntity(e);
		}
	}
}

void EC_Engine::removeEntity(unsigned int entityID)
{
	for (size_t i = 0; i < m_Systems.size(); i++)
	{
		if (m_Systems[i])
		{
			m_Systems[i]->removeEntity(entityID);
		}
	}
}

void EC_Engine::clearEntities()
{
	for (size_t i = 0; i < m_Systems.size(); i++)
	{
		if (m_Systems[i])
		{
			m_Systems[i]->clearEntities();
		}
	}
}

void EC_Engine::pause()
{
	for (auto t : m_tasks)
	{
		t->pause();
	}
}

void EC_Engine::resume()
{
	for (auto t : m_tasks)
	{
		t->resume();
	}
}


EC_Engine::~EC_Engine()
{
}

void EC_Engine::receive(ECXCommand& command)
{
	if (command.type == ECXCommandType::SystemStart)
	{
		start();
	}
	else if (command.type == ECXCommandType::SystemShutdown)
	{
		stop();
	}
}
