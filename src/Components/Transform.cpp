#include "Transform.h"
#include <glm\gtc\matrix_transform.hpp>


Transform::Transform()
{
	m_Scale = glm::vec3(1.0f);
}


Transform::~Transform()
{
	m_Owner = nullptr;
}

ComponentType Transform::getComponentType()
{
	return ComponentType::Transform;
}

void Transform::setTransform(glm::mat4 transform)
{
	m_Transform = transform;
	notifyObservers();
}

void Transform::setScale(glm::vec3 scale)
{
	m_Scale = scale;
	notifyObservers();
}

glm::vec3 Transform::getScale()
{
	return m_Scale;
}

glm::mat4& Transform::getTransform()
{
	return m_Transform;
}
