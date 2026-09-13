#include "Engine/Controllers/Controller.h"



Controller::Controller()
{
	m_running = true;
}


void Controller::shutdown()
{
	m_running = false;
}

Controller::~Controller()
{
}
