#include "Spatial.h"
#include "Entity/GameEntity.h"


Spatial::Spatial()
{
	m_Position = glm::vec3(0.0f);
	m_Velocity = glm::vec3(0.0f);
	m_EulerOrientation = glm::vec3(0.0f);
	m_AngularVelocity = glm::vec3(0.0f);
}


Spatial::~Spatial()
{
	m_Owner = nullptr;
}

ComponentType Spatial::getComponentType()
{
	return ComponentType::Spatial;
}

void Spatial::setPosition(glm::vec3 position)
{
	m_Position = position;
	notifyObservers();
}

glm::vec3 Spatial::getPosition()
{
	return m_Position;
}

void Spatial::setVelocity(glm::vec3 velocity)
{
	m_Velocity = velocity;
	notifyObservers();
}

glm::vec3 Spatial::getVelocity()
{
	return m_Velocity;
}

void Spatial::setOrientation(glm::vec3 orientation)
{
	m_EulerOrientation = orientation;
	notifyObservers();
}

glm::vec3 Spatial::getOrientation()
{
	return m_EulerOrientation;
}

void Spatial::setAngVelocity(glm::vec3 angV)
{
	m_AngularVelocity = angV;
	notifyObservers();
}

glm::vec3 Spatial::getAngVelocity()
{
	return m_AngularVelocity;
}

void Spatial::setDirections(glm::vec3 & forward, glm::vec3 & up, glm::vec3 right)
{
	m_Forward = forward;
	m_Up = up;
	m_Right = right;
	notifyObservers();
}

glm::vec3 Spatial::getForward()
{
	return m_Forward;
}

glm::vec3 Spatial::getUp()
{
	return m_Up;
}

glm::vec3 Spatial::getRight()
{
	return m_Right;
}
