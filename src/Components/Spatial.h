#pragma once
#include "IComponent.h"
#include <glm\glm.hpp>
class GameEntity;
class Spatial :
	public IComponent
{
public:
	Spatial();
	virtual ~Spatial();

	// Inherited via IComponent
	virtual ComponentType getComponentType() override;
	void setPosition(glm::vec3 position);
	glm::vec3 getPosition();
	void setVelocity(glm::vec3 velocity);
	glm::vec3 getVelocity();
	void setOrientation(glm::vec3 orientation);
	glm::vec3 getOrientation();
	void setAngVelocity(glm::vec3 angV);
	glm::vec3 getAngVelocity();
	void setDirections(glm::vec3& forward, glm::vec3& up, glm::vec3 right);
	glm::vec3 getForward();
	glm::vec3 getUp();
	glm::vec3 getRight();
private:
	glm::vec3 m_Position;
	glm::vec3 m_Velocity;
	glm::vec3 m_EulerOrientation;
	glm::vec3 m_AngularVelocity;
	glm::vec3 m_Forward;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
};

