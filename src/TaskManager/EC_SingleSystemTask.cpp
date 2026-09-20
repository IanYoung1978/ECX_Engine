#include "EC_SingleSystemTask.h"
#include "Engine/Subsystems/EC_System.h"

EC_SingleSystemTask::EC_SingleSystemTask()
{
	m_game = nullptr;
	m_current_time = 0;
	m_timestep = 0.0f;
	m_accumulator = 0.0f;
}

EC_SingleSystemTask::~EC_SingleSystemTask()
{
}

void EC_SingleSystemTask::addSystem(std::shared_ptr<EC_System> system)
{
	m_system = system;
}

void EC_SingleSystemTask::addGameRef(EC_Game& game)
{
	m_game = &game;
}

void EC_SingleSystemTask::setTimeStep(float timestep)
{
	m_timestep = timestep;
}

void EC_SingleSystemTask::execute()
{
	while (m_running)
	{
		clock_t latest = clock();
		clock_t dt_clocks = latest - m_current_time;
		float dt = (float)dt_clocks / CLOCKS_PER_SEC;
		if (m_accumulator >= m_timestep)
		{
			if (!m_Paused)
			{
				m_system->update(m_timestep, *m_game);
				recordTick();
			}
			m_accumulator = 0.0f;
		}
		else
			m_accumulator += dt;
		m_current_time = latest;
	}
}
