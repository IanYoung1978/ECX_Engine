#include "EC_SpatialSystem.h"
#include "Components/Spatial.h"
#include "Components/Transform.h"
#include <glm\gtc\matrix_transform.hpp>
#include "Common/EC_Subject.h"
#include <glm\gtc\quaternion.hpp>
#include <iostream>
#include "Components/ProxyHelper.h"
#include "Entity/EntityManager.h"

EC_SpatialSystem::EC_SpatialSystem()
{
}


EC_SpatialSystem::~EC_SpatialSystem()
{
}

void EC_SpatialSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	// not used
}

void EC_SpatialSystem::update(const float & deltaTimeS, EC_Game& game)
{
	auto entities = EntityManager::getInstance().getEntitiesWithComponent(std::type_index(typeid(Spatial)));

	for (size_t i = 0; i < entities.size(); i++)
	{
		auto spatial = entities[i]->getComponent<Spatial>();
		//compute changes
		auto position = spatial->getPosition();
		auto orientation = spatial->getOrientation();
		position += spatial->getVelocity()*deltaTimeS;
		orientation += spatial->getAngVelocity()*deltaTimeS;

		glm::vec3 Direction;
		Direction.x = cos(orientation.x) * sin(orientation.y);
		Direction.y = sin(orientation.x);
		Direction.z = cos(orientation.x) * cos(orientation.y);
		Direction = glm::normalize(Direction);
		glm::vec3 Right = glm::normalize(glm::cross(Direction, glm::vec3(0.0f,1.0f,0.0f)));
		glm::vec3 Up = glm::normalize(glm::cross(Right, Direction));
		//update components
		spatial->setOrientation(orientation);
		spatial->setPosition(position);
		spatial->setDirections(Direction, Up, Right);

	}

}