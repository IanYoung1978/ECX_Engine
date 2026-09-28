#pragma once
#include "EC_SystemTask.h"
#include <memory>
#include <ctime>

class EC_Game;
class EC_System;

// Runs exactly one system on its own thread (Scripting and Audio each get one).
class EC_SingleSystemTask :
	public EC_SystemTask
{
public:
	EC_SingleSystemTask();
	virtual ~EC_SingleSystemTask();
	void addSystem(std::shared_ptr<EC_System> system);
	void addGameRef(EC_Game& game);
	// Fixed-timestep accumulator, same pattern as EC_PhysicsThreadTask - without this the
	// system would either get a bogus hardcoded dt (this class's own prior bug: every call
	// passed exactly 1.0f regardless of real elapsed time) or busy-spin calling update()
	// as fast as possible with whatever the real, unthrottled per-call dt happens to be.
	void setTimeStep(float timestep);

	// Inherited via EC_Task
	virtual void execute() override;
private:
	std::shared_ptr<EC_System> m_system;
	EC_Game* m_game;
	clock_t m_current_time;
	float m_timestep;
	float m_accumulator;
};
