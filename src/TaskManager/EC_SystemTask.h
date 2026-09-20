#pragma once
#include "EC_Task.h"
#include <atomic>
#include <ctime>
#include <string>

class EC_SystemTask :
	public EC_Task
{
public:
	EC_SystemTask();
	void start();
	void stop();
	void pause();
	void resume();
	virtual ~EC_SystemTask();

	// A label for the /threadfps debug-HTTP route ("Physics", "Scripting", "Audio", ...) -
	// purely descriptive, set once by whoever constructs the task (see EC_Engine::init()).
	void setName(const std::string& name) { m_Name = name; }
	const std::string& getName() const { return m_Name; }

	// How many real updates (not raw spin-loop iterations) this task's execute() completed
	// in roughly the last second - lets an author see which of the engine's fixed-timestep
	// threads (Physics, Scripting, Audio) is actually keeping up with its configured
	// timestep and which is the one throttled, rather than guessing from the main thread's
	// own FPS alone (see EC_Game::getFPS()/game:getFPS() for that). Every derived task's
	// execute() calls recordTick() exactly once per real update it performs.
	float getTickRate() const { return m_TickRate.load(std::memory_order_relaxed); }

protected:
	void recordTick();

	bool m_Paused;
	bool m_running;

private:
	std::string m_Name;
	std::atomic<float> m_TickRate{ 0.0f };
	size_t m_TickCount = 0;
	clock_t m_TickWindowStart = 0;
};

