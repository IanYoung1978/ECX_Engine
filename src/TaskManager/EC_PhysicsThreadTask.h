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
	// Collision + Physics only - run once per visual tick isn't enough for
	// stable stacking (see setSubstepCount below); Spatial/Transform/Camera
	// still only need the once-per-tick addSystem() above. Scripting and
	// Audio do NOT belong on this task at all - each gets its own dedicated
	// thread via EC_SingleSystemTask instead (see EC_Engine::init()) so
	// their per-frame work, and anything a script touches through them
	// (e.g. EC_SceneManager's entity lookup maps), is never touched
	// concurrently from this thread and the main thread at the same time.
	void addSubsteppedSystem(std::shared_ptr<EC_System> system);
	void addGameRef(EC_Game& game);
	void setTimeStep(float timestep);
	// How many substeps each visual tick's Collision+Physics work is split
	// into - the standard fix real physics engines (Box2D v3's TGS Soft,
	// Rapier) use for exactly the kind of slow, marginal-equilibrium creep
	// a stack shows when a hit only barely destabilizes it: each substep
	// re-detects collisions against the CURRENT (already-moved-this-tick)
	// positions and re-solves against a smaller effective timestep, so
	// gravity/torque integration tracks the true continuous-time dynamics
	// far more closely than one big low-frequency correction per visual
	// frame - the discretization error that shows up as "boxes slowly
	// sliding/dancing before finally toppling" shrinks with the substep
	// size. Defaults to 1 (no substepping) if never called.
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

