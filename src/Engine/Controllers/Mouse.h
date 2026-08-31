#pragma once
#include "Engine/Controllers/Controller.h"
#include <vector>
#include <glm\glm.hpp>
#include "Engine/Controllers/MouseButton.h"
class Mouse :
	public Controller
{
public:
	Mouse();
	virtual ~Mouse();

	// Inherited via Controller
	virtual void update(ECXMessenger& messenger) override;
	virtual void handleEvent(SDL_Event & e) override;
	virtual bool keyPressed(SDL_Scancode key) override;
	virtual bool keyHeld(SDL_Scancode key) override;
	virtual bool keyReleased(SDL_Scancode key) override;
	bool keyPressed(MouseButton key);
	bool keyHeld(MouseButton key);
	bool keyReleased(MouseButton key);
	void setFPSMode(bool toggle = true);
	virtual ControllerType getControllerType() override;
	glm::ivec2 getMouseMovement();
	glm::ivec2 getMousePosition() const { return m_mouse_position; }
	virtual bool changed() override;
	virtual KeyState getKeyState(SDL_Scancode key) override;
private:
	static const int Num_Mouse_Buttons = 5;
	bool m_PressedBuffer[Num_Mouse_Buttons];
	bool m_HeldBuffer[Num_Mouse_Buttons];
	glm::ivec2 m_MouseMotion;
	glm::ivec2 m_LastMouseMotion;
	glm::ivec2 m_mouse_position;
	bool m_FPS_Mode;
	bool m_Changed;
};

