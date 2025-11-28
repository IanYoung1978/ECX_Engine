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
	void addGameRef(EC_Game& game);
	void setTimeStep(float timestep);

	// Inherited via EC_Task
	virtual void execute() override;
private:
	std::vector<std::shared_ptr<EC_System>> m_Systems;
	EC_Game* m_game;
	clock_t m_current_time;
	float m_timestep;
	float m_accumulator;
};

