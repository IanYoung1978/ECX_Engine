#include "MouseEvent.h"



MouseEvent::MouseEvent(MouseButton data, bool held, bool pressed, int x_pos, int y_pos, int x_motion, int y_motion)
{
	m_MouseEventData = data;
	m_ButtonPressed = pressed;
	m_ButtonHeld = held;
	m_X_Motion = x_motion;
	m_Y_Motion = y_motion;
	m_Xpos = x_pos;
	m_Ypos = y_pos;
	m_MouseMoved = (data == MouseButton::Motion);
}


MouseEvent::~MouseEvent()
{
}

MouseButton MouseEvent::getMouseKey()
{
	return m_MouseEventData;
}

bool MouseEvent::buttonHeld()
{
	return m_ButtonHeld;
}

bool MouseEvent::buttonReleased()
{
	return (!m_ButtonHeld && !m_ButtonPressed);
}

bool MouseEvent::buttonPressed()
{
	return m_ButtonPressed;
}

bool MouseEvent::mouseMoved()
{
	return m_MouseMoved;
}

int MouseEvent::getXMotion()
{
	return m_X_Motion;
}

int MouseEvent::getYMotion()
{
	return m_Y_Motion;
}

void MouseEvent::getPosition(int & x, int & y)
{
	x = m_Xpos;
	y = m_Ypos;
}