#include "Engine/Subsystems/EC_CameraSystem.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include <glm/gtc/matrix_transform.hpp>

EC_CameraSystem::EC_CameraSystem() {
}

EC_CameraSystem::~EC_CameraSystem() {
}

void EC_CameraSystem::init(ECXMessenger& messenger, EC_Game& game) {
}

void EC_CameraSystem::update(const float& deltaTimeS, EC_Game& game) {
    auto& manager = EC_DOD_EntityManager::getInstance();

    auto entities = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Spatial)),
        std::type_index(typeid(EC_DOD_Camera))
        });

    for (EntityID entityID : entities) {
        if (!manager.isAlive(entityID)) {
            continue;
        }

        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(entityID);
        auto& camera = manager.getComponent<EC_DOD_Camera>(entityID);

        glm::mat4 view = glm::lookAt(
            spatial.position,
            spatial.position + spatial.direction,
            spatial.up
        );

        camera.viewMatrix = view;
    }
}