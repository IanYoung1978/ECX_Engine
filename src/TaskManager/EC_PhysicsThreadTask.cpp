#include "EC_PhysicsThreadTask.h"



EC_PhysicsThreadTask::EC_PhysicsThreadTask()
{
	m_timestep = 0.0f;
	m_game = nullptr;
	m_current_time = 0;
	m_accumulator = 0.0f;
}


EC_PhysicsThreadTask::~EC_PhysicsThreadTask()
{
}

void EC_PhysicsThreadTask::addSystem(std::shared_ptr<EC_System> system)
{
	m_Systems.push_back(system);
}

void EC_PhysicsThreadTask::addGameRef(EC_Game & game)
{
	m_game = &game;
}

void EC_PhysicsThreadTask::setTimeStep(float timestep)
{
	m_timestep = timestep;
}


void EC_PhysicsThreadTask::execute()
{
	while (m_running)
	{
		clock_t m_latest = clock();
		clock_t dt_clocks = m_latest - m_current_time;
		float dt = (float)dt_clocks / CLOCKS_PER_SEC;
		if (m_accumulator >= m_timestep)
		{
			if (!m_Paused)
			{
				for (auto s : m_Systems)
				{
					s->update(m_timestep, *m_game);
				}
			}
			m_accumulator = 0.0f;
		}
		else
			m_accumulator += dt;
		m_current_time = m_latest;
	}
}

