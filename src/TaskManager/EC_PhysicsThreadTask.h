#pragma once
#include "EC_SystemTask.h"
#include "Engine/Subsystems/EC_System.h"
#include <vector>
#include <ctime>
class EC_Game;
class EC_PhysicsThreadTask :
	public EC_SystemTask
{
public:
	EC_PhysicsThreadTask();
	virtual ~EC_PhysicsThreadTask();
	void addSystem(std::shared_ptr<EC_System> system);
	// Collision + Physics only, run once per substep; Spatial/Transform/Camera use addSystem() above.
	// Scripting and Audio don't belong on this task: each gets its own thread via EC_SingleSystemTask
	// (see EC_Engine::init()).
	void addSubsteppedSystem(std::shared_ptr<EC_System> system);
	void addGameRef(EC_Game& game);
	void setTimeStep(float timestep);
	// Collision+Physics passes per visual tick, each with a proportionally smaller timestep.
	// Re-detecting against already-moved positions each pass fixes the slow creep of marginally
	// balanced stacks. Defaults to 1 (no substepping).
	void setSubstepCount(int count) { m_SubstepCount = std::max(1, count); }

	// Inherited via EC_Task
	virtual void execute() override;
private:
	std::vector<std::shared_ptr<EC_System>> m_Systems;
	std::vector<std::shared_ptr<EC_System>> m_SubsteppedSystems;
	EC_Game* m_game;
	clock_t m_current_time;
	float m_timestep;
	float m_accumulator;
	int m_SubstepCount = 1;
};

