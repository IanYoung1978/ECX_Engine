#include "Engine/Subsystems/Camera/EC_CameraSystem.h"
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

    // Active+sceneState filtered, matching EC_LuaScriptSystem/EC_BroadPhase/EC_PhysicsSystem:
    // an inactive scene's cameras shouldn't keep computing view matrices (wasted work at
    // best; since a deactivated scene's entities stay alive in memory through their
    // quarantine period rather than being destroyed immediately, an indefinitely-inactive
    // scene's camera must never be mistaken for a live one here).
    auto entities = manager.getActiveEntitiesWithComponents({
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