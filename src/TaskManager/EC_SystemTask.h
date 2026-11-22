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

	// A label for this task ("Physics", "Scripting", "Audio", ...), set by whoever constructs it (see
	// EC_Engine::init()).
	void setName(const std::string& name) { m_Name = name; }
	const std::string& getName() const { return m_Name; }

	// Real updates completed in roughly the last second (not spin iterations), so an author can see
	// which fixed-timestep thread is falling behind. Each derived task's execute() calls recordTick()
	// once per real update.
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

