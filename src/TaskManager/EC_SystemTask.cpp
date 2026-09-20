#include "EC_SystemTask.h"



EC_SystemTask::EC_SystemTask()
{
	m_running = false;
	m_Paused = false;
}

void EC_SystemTask::start()
{
	m_running = true;
}

void EC_SystemTask::stop()
{
	m_running = false;
}

void EC_SystemTask::pause()
{
	m_Paused = true;
}

void EC_SystemTask::resume()
{
	m_Paused = false;
}

void EC_SystemTask::recordTick()
{
	if (m_TickWindowStart == 0)
	{
		m_TickWindowStart = clock();
		return;
	}

	m_TickCount++;
	clock_t elapsed = clock() - m_TickWindowStart;
	float elapsedSeconds = static_cast<float>(elapsed) / CLOCKS_PER_SEC;
	if (elapsedSeconds >= 1.0f)
	{
		m_TickRate.store(static_cast<float>(m_TickCount) / elapsedSeconds, std::memory_order_relaxed);
		m_TickCount = 0;
		m_TickWindowStart = clock();
	}
}

EC_SystemTask::~EC_SystemTask()
{
}
