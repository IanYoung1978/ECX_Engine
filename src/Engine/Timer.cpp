#include "Timer.h"
#include "Game.h"


Timer::Timer()
{
	m_Current = clock();
	m_Last = m_Current;
	m_Delta = 0;
	m_DTS = 0.0f;
}

void Timer::update(EC_Game & game)
{
	m_Current = clock();
	m_Delta = m_Current - m_Last;
	m_DTS = (float)m_Delta/CLOCKS_PER_SEC;
	m_Last = m_Current;
	game.update(m_DTS);
}


Timer::~Timer()
{
}
