#pragma once
#include <time.h>
class EC_Game;
class Timer
{
public:
	Timer();
	void update(EC_Game& game);
	float getFPS() const { return m_FPS; }
	float getMSPF() const { return m_MSPF; }
	~Timer();
private:
	clock_t m_Last;
	clock_t m_Current;
	clock_t m_Delta;
	float m_DTS;
	float m_FPS = 0.0f;
	float m_MSPF = 0.0f;
	static constexpr float kFPSSmoothing = 0.9f; // EMA factor, higher = smoother/slower to react
};

