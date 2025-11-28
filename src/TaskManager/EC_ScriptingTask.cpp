#include "EC_ScriptingTask.h"
#include "Engine/Subsystems/EC_System.h"


EC_ScriptingTask::EC_ScriptingTask()
{
	m_game = nullptr;
}

void EC_ScriptingTask::addSystem(std::shared_ptr<EC_System> system)
{
	m_system = system;
}

void EC_ScriptingTask::addGameRef(EC_Game& game)
{
	m_game = &game;
}

EC_ScriptingTask::~EC_ScriptingTask()
{
}

void EC_ScriptingTask::execute()
{
	while (m_running)
	{
		m_system->update(1.0f, *m_game);
	}
}
