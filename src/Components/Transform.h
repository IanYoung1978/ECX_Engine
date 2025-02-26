#pragma once
#include "IComponent.h"
#include <glm\glm.hpp>
class Transform :
	public IComponent
{
public:
	Transform();
	virtual ~Transform();
	// Inherited via IComponent
	virtual ComponentType getComponentType() override;
	void setTransform(glm::mat4 transform);
	void setScale(glm::vec3 scale);
	glm::vec3 getScale();
	glm::mat4 &getTransform();
private:
	glm::mat4 m_Transform;
	std::shared_ptr<GameEntity> m_Owner;
	glm::vec3 m_Scale;
};

