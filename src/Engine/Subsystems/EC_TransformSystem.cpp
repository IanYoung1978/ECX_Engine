#include "Engine/Subsystems/EC_TransformSystem.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include <glm/gtc/matrix_transform.hpp>

EC_TransformSystem::EC_TransformSystem() {
}

EC_TransformSystem::~EC_TransformSystem() {
}

void EC_TransformSystem::init(ECXMessenger& messenger, EC_Game& game) {
}

void EC_TransformSystem::update(const float& deltaTimeS, EC_Game& game) {
    auto& manager = EC_DOD_EntityManager::getInstance();

    auto entities = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Spatial)),
        std::type_index(typeid(EC_DOD_Transform))
        });

    for (EntityID entityID : entities) {
        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(entityID);
        auto& transform = manager.getComponent<EC_DOD_Transform>(entityID);

        glm::mat4 trans = glm::translate(glm::mat4(1.0f), spatial.position);

        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, spatial.orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, spatial.orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, spatial.orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));

        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
        if (transform.localTransform != glm::mat4(1.0f)) {
            scaleMatrix = transform.localTransform;
        }

        transform.worldTransform = trans * rotation * scaleMatrix;
        transform.dirty = false;
    }
}