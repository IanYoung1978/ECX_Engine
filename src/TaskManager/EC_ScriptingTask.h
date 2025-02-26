#pragma once
#include "EC_SystemTask.h"
#include <memory>

class EC_Game;
class EC_System;
class EC_ScriptingTask :
	public EC_SystemTask
{
public:
	EC_ScriptingTask();
	void addSystem(std::shared_ptr<EC_System> system);
	void addGameRef(EC_Game& game);
	virtual ~EC_ScriptingTask();
private:
	std::shared_ptr<EC_System> m_system;
	EC_Game* m_game;

	// Inherited via EC_SystemTask
	virtual void execute() override;
};

