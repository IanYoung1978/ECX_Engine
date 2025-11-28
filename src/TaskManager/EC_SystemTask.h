#pragma once
#include "EC_Task.h"
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
protected:
	bool m_Paused;
	bool m_running;
};

