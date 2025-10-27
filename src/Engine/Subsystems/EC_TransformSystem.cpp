#include "EC_TransformSystem.h"
#include <glm\gtc\matrix_transform.hpp>
#include <mutex>
#include "Components/ProxyHelper.h"
#include "Entity/EntityManager.h"


EC_TransformSystem::EC_TransformSystem()
{
}


EC_TransformSystem::~EC_TransformSystem()
{
}

void EC_TransformSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	//not used
}

void EC_TransformSystem::update(const float & deltaTimeS, EC_Game & game)
{
	auto entities = EntityManager::getInstance().getEntitiesWithComponents({ 
		std::type_index(typeid(Spatial)), 
		std::type_index(typeid(Transform)) 
	});

	for (auto& e : entities)
	{
		auto spatial = e->getComponent<Spatial>();
		auto transform = e->getComponent<Transform>();
		auto orientation = spatial->getOrientation();
		auto position = spatial->getPosition();
		glm::mat4 trans(1.0f);
		trans = glm::translate(trans, position);
		glm::mat4 rotation(1.0f);
		rotation = glm::rotate(rotation, orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		rotation = glm::rotate(rotation, orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		rotation = glm::rotate(rotation, orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 scale(1.0f);
		scale = glm::scale(scale, transform->getScale());
		glm::mat4 matrix = trans * rotation * scale;
		transform->setTransform(matrix);
	}
}
