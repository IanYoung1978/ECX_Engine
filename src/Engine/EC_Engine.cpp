#include "EC_Engine.h"
#include "Engine/Subsystems/EC_SpatialSystem.h"
#include "Engine/Subsystems/EC_TransformSystem.h"
#include "Engine/Subsystems/EC_CameraSystem.h"
#include "Engine/Subsystems/EC_LuaScriptingSystem.h"
#include "Engine/Subsystems/CollisionSystems/EC_CollisionSystem.h"
#include "Engine/Subsystems/EC_PhysicsSystem.h"
#include "TaskManager/EC_PhysicsThreadTask.h"
#include "TaskManager/EC_ScriptingTask.h"
#include "xml/XML.h"

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
	m_Systems[(size_t)EC_SystemType::Collision] = std::make_shared<EC_CollisionSystem>();
	m_Systems[(size_t)EC_SystemType::Physics] = std::make_shared<EC_PhysicsSystem>();
	m_Systems[(size_t)EC_SystemType::Scripting] = std::make_shared<EC_LuaScriptSystem>();

	for (auto s : m_Systems)
	{
		if (s != nullptr)
		{
			s->init(messenger, game);
		}
	}

	// Physics reads collision manifolds cached on the pair manager
	// EC_CollisionSystem's broad/narrow phase populate - wire it once here,
	// after both systems exist, so it never has to guess at the instance.
	auto* physicsSystem = static_cast<EC_PhysicsSystem*>(m_Systems[(size_t)EC_SystemType::Physics].get());
	physicsSystem->setPairManager(
		&static_cast<EC_CollisionSystem*>(m_Systems[(size_t)EC_SystemType::Collision].get())->getPairManager());

	XML::PhysicsDebugSettings debugSettings;
	int substeps = 1;
	XML::loadPhysicsDebugSettings(config, debugSettings);
	XML::loadPhysicsSubstepCount(config, substeps);
	physicsSystem->setDebugLogging(debugSettings.logEnergy, debugSettings.logVelocity,
		debugSettings.logAngularVelocity, debugSettings.logFriction);

	auto task = std::make_shared<EC_PhysicsThreadTask>();
	task->addGameRef(*m_game);
	task->addSystem(m_Systems[(size_t)EC_SystemType::Spatial]);
	task->addSystem(m_Systems[(size_t)EC_SystemType::Transform]);
	task->addSystem(m_Systems[(size_t)EC_SystemType::Camera]);
	task->addSystem(m_Systems[(size_t)EC_SystemType::Scripting]);
	// Collision + Physics are substepped instead - see
	// EC_PhysicsThreadTask::setSubstepCount for why (stacking stability:
	// the same fix Box2D v3/Rapier use for marginal-equilibrium creep).
	// Must stay in this relative order (Collision before Physics) within
	// each substep, since Physics consumes the manifolds Collision just
	// cached that same substep.
	task->addSubsteppedSystem(m_Systems[(size_t)EC_SystemType::Collision]);
	task->addSubsteppedSystem(m_Systems[(size_t)EC_SystemType::Physics]);
	task->setSubstepCount(substeps);
	task->setTimeStep(1.0f / 60);
	m_tasks.push_back(task);

	LOGGING::ECX_Logger::GetInstance()->LogMessage("Engine initialised", LOGGING::LogLevel::INFORMATION);
}

void EC_Engine::start()
{
	for (auto task : m_tasks)
	{
		task->start();
		m_threadpool.addTask(task);
	}
	m_threadpool.executeTasks();
}

void EC_Engine::stop()
{
	for (auto t : m_tasks)
	{
		t->stop();
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

void EC_Engine::stepOnce(float deltaTimeS)
{
	for (auto s : m_Systems)
	{
		if (s != nullptr)
		{
			s->update(deltaTimeS, *m_game);
		}
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