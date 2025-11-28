#pragma once
#include "Engine/MouseButton.h"

class MouseEvent
{
public:
	MouseEvent(MouseButton data, bool held, bool pressed, int x_pos = 0, int y_pos = 0, int x_motion = 0, int y_motion = 0);
	~MouseEvent();
	MouseButton getMouseKey();
	bool buttonHeld();
	bool buttonReleased();
	bool buttonPressed();
	bool mouseMoved();
	int getXMotion();
	int getYMotion();
	void getPosition(int& x, int& y);

private:
	MouseButton m_MouseEventData;
	bool m_ButtonHeld;
	bool m_ButtonPressed;
	int m_X_Motion;
	int m_Y_Motion;
	int m_Xpos;
	int m_Ypos;
	bool m_MouseMoved;
};

