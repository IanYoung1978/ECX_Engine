#include "EC_Engine.h"
#include "Engine/Subsystems/Spatial/EC_SpatialSystem.h"
#include "Engine/Subsystems/Transform/EC_TransformSystem.h"
#include "Engine/Subsystems/Camera/EC_CameraSystem.h"
#include "Engine/Subsystems/Scripting/EC_LuaScriptingSystem.h"
#include "Procedural/EC_VolumeNode.h"
#include "Engine/Subsystems/CollisionSystems/EC_CollisionSystem.h"
#include "Engine/Subsystems/CollisionSystems/EC_PhysicsSystem.h"
#include "Engine/Subsystems/Audio/EC_AudioSystem.h"
#include "TaskManager/EC_PhysicsThreadTask.h"
#include "TaskManager/EC_SingleSystemTask.h"
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
	m_Systems[(size_t)EC_SystemType::Audio] = std::make_shared<EC_AudioSystem>();

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
	task->setName("Physics");
	m_tasks.push_back(task);

	// Scripting and Audio each get their own dedicated thread - NOT the physics thread
	// above (that class is scoped to Collision+Physics; see its own header comment).
	// Scripting was always supposed to be on its own thread - EC_SingleSystemTask (this
	// same generic task class) existed for exactly this purpose but was never actually
	// instantiated anywhere, and Scripting had been folded onto the physics thread
	// instead. That meant the Lua state, and everything a script touches through it (e.g.
	// EC_SceneManager's entity lookup maps, populated on the main thread right after a
	// scene loads), was being read/written from two threads at once with no synchronization
	// - the real cause of a "cannot dereference value-initialized iterator" crash found
	// investigating #135, not fixable by patching individual containers one at a time.
	auto scriptingTask = std::make_shared<EC_SingleSystemTask>();
	scriptingTask->addGameRef(*m_game);
	scriptingTask->addSystem(m_Systems[(size_t)EC_SystemType::Scripting]);
	scriptingTask->setTimeStep(1.0f / 60);
	scriptingTask->setName("Scripting");
	m_tasks.push_back(scriptingTask);

	// Audio's update() is just cheap bookkeeping (reaping finished one-shot sounds) - the
	// actual mixing/playback already runs on miniaudio's own dedicated engine thread
	// regardless of which thread calls into EC_AudioSystem - but it still needs its own
	// thread here, not the physics thread, for the same reason as Scripting above.
	auto audioTask = std::make_shared<EC_SingleSystemTask>();
	audioTask->addGameRef(*m_game);
	audioTask->addSystem(m_Systems[(size_t)EC_SystemType::Audio]);
	audioTask->setTimeStep(1.0f / 60);
	audioTask->setName("Audio");
	m_tasks.push_back(audioTask);

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

std::vector<EC_Engine::ThreadRate> EC_Engine::getThreadRates() const
{
	std::vector<ThreadRate> rates;
	rates.reserve(m_tasks.size());
	for (const auto& task : m_tasks)
	{
		rates.push_back({ task->getName(), task->getTickRate() });
	}
	return rates;
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

bool EC_Engine::runLuaScriptOnce(const std::string& filename)
{
	auto* scripting = static_cast<EC_LuaScriptSystem*>(m_Systems[(size_t)EC_SystemType::Scripting].get());
	return scripting ? scripting->runScriptOnce(filename) : false;
}

std::shared_ptr<EC_VolumeNode> EC_Engine::getVolumeRoot() const
{
	auto* scripting = static_cast<EC_LuaScriptSystem*>(m_Systems[(size_t)EC_SystemType::Scripting].get());
	return scripting ? scripting->getVolumeRoot() : nullptr;
}

ScriptAPI::VoxelTerrainConfig EC_Engine::getVoxelTerrainConfig() const
{
	auto* scripting = static_cast<EC_LuaScriptSystem*>(m_Systems[(size_t)EC_SystemType::Scripting].get());
	return scripting ? scripting->getVoxelTerrainConfig() : ScriptAPI::VoxelTerrainConfig{};
}

EC_AudioSystem* EC_Engine::getAudioSystem() const
{
	return static_cast<EC_AudioSystem*>(m_Systems[(size_t)EC_SystemType::Audio].get());
}

void EC_Engine::playSound(const std::string& path, float volume, const std::string& category)
{
	if (auto* audio = getAudioSystem()) audio->playSound(path, volume, category);
}

void EC_Engine::playMusic(const std::string& path, float volume, bool loop)
{
	if (auto* audio = getAudioSystem()) audio->playMusic(path, volume, loop);
}

void EC_Engine::stopMusic()
{
	if (auto* audio = getAudioSystem()) audio->stopMusic();
}

void EC_Engine::setCategoryVolume(const std::string& category, float volume)
{
	if (auto* audio = getAudioSystem()) audio->setCategoryVolume(category, volume);
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