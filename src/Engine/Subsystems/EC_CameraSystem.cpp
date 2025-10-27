#include "EC_CameraSystem.h"
#include "Components/EC_CameraComponent.h"
#include "Components/Spatial.h"
#include <glm\gtc\matrix_transform.hpp>
#include "Entity/GameEntity.h"
#include "Components/ProxyHelper.h"
#include "Entity/EntityManager.h"

EC_CameraSystem::EC_CameraSystem()
{
}


EC_CameraSystem::~EC_CameraSystem()
{
}

void EC_CameraSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	// not used
}

void EC_CameraSystem::update(const float & deltaTimeS, EC_Game & game)
{

	auto entities = EntityManager::getInstance().getEntitiesWithComponents({
		std::type_index(typeid(Spatial)),
		std::type_index(typeid(EC_CameraComponent))
		});

	for (auto& e : entities)
	{
		if (e->isActive())
		{

			glm::vec3 pos = e->getComponent<Spatial>()->getPosition();
			glm::vec3 forward = e->getComponent<Spatial>()->getForward();
			glm::vec3 up = e->getComponent<Spatial>()->getUp();
			glm::mat4 view = glm::lookAt(pos, pos + forward, up);
			e->getComponent<EC_CameraComponent>()->setViewMatrix(view);
		}
	}
}