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


EC_SystemTask::~EC_SystemTask()
{
}
