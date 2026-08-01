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

void EC_PhysicsThreadTask::addSubsteppedSystem(std::shared_ptr<EC_System> system)
{
	m_SubsteppedSystems.push_back(system);
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

				// Collision + Physics run m_SubstepCount times against a
				// proportionally smaller timestep instead of once at full
				// m_timestep - each pass re-detects collisions against
				// whatever the previous pass just integrated, so a body
				// mid-topple gets its geometry/impulses refreshed several
				// times within one visual tick rather than once. See
				// setSubstepCount's declaration for why this specifically
				// fixes marginal-equilibrium creep.
				const float substepDt = m_timestep / static_cast<float>(m_SubstepCount);
				for (int i = 0; i < m_SubstepCount; i++)
				{
					for (auto s : m_SubsteppedSystems)
					{
						s->update(substepDt, *m_game);
					}
				}
			}
			m_accumulator = 0.0f;
		}
		else
			m_accumulator += dt;
		m_current_time = m_latest;
	}
}

