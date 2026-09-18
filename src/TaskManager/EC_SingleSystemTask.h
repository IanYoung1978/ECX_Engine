#pragma once
#include "EC_SystemTask.h"
#include <memory>
#include <ctime>

class EC_Game;
class EC_System;

// A dedicated background thread for exactly one system - used to keep Scripting and Audio
// each on their own thread, off EC_PhysicsThreadTask entirely (that class is scoped to
// Collision+Physics, not a general-purpose task runner - see its own header comment).
// Genuinely generic despite living in TaskManager/ (this class was previously named
// EC_ScriptingTask, single-purpose in name only - it never referenced anything
// scripting-specific - and was never actually instantiated anywhere: Scripting had been
// folded onto the physics thread instead, the exact mistake this class exists to avoid).
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
