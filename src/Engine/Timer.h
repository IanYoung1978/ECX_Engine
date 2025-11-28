#pragma once
#include <time.h>
class EC_Game;
class Timer
{
public:
	Timer();
	void update(EC_Game& game);
	~Timer();
private:
	clock_t m_Last;
	clock_t m_Current;
	clock_t m_Delta;
	float m_DTS;
};

