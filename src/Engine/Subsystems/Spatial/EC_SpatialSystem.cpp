#include "Engine/Subsystems/Spatial/EC_SpatialSystem.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

EC_SpatialSystem::EC_SpatialSystem() {
}

EC_SpatialSystem::~EC_SpatialSystem() {
}

void EC_SpatialSystem::init(ECXMessenger& messenger, EC_Game& game) {
}

// Position integration runs here for every entity with a Spatial. Orientation is owned by
// EC_PhysicsSystem's quaternion integration for Dynamic bodies only; everything else (Kinematic
// included) uses the Euler accumulation below.
void EC_SpatialSystem::update(const float& deltaTimeS, EC_Game& game) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    auto* spatialArray = manager.getComponentArray<EC_DOD_Spatial>();

    if (!spatialArray) {
        return;
    }

    std::shared_lock lock(spatialArray->getMutex());
    auto& spatials = spatialArray->getData();

    for (size_t i = 0; i < spatials.size(); i++) {
        auto& spatial = spatials[i];

        EntityID entity = spatialArray->getEntityUnlocked(i);

        // Active+sceneState filtered, matching every other system - see EC_CameraSystem.cpp's
        // own comment for why.
        if (manager.hasComponent<EC_DOD_EntityInfo>(entity)) {
            const auto& info = manager.getComponent<EC_DOD_EntityInfo>(entity);
            if (!info.active || info.sceneState != EC_SceneLifecycleState::Active) continue;
        }

        spatial.position += spatial.velocity * deltaTimeS;

        bool physicsOwnsOrientation = manager.hasComponent<EC_DOD_RigidBody>(entity) &&
            manager.getComponent<EC_DOD_RigidBody>(entity).bodyType == EC_BodyType::Dynamic;
        if (physicsOwnsOrientation) {
            continue;
        }

        spatial.orientation += spatial.angVelocity * deltaTimeS;

        glm::vec3 direction;
        direction.x = cos(spatial.orientation.x) * sin(spatial.orientation.y);
        direction.y = sin(spatial.orientation.x);
        direction.z = cos(spatial.orientation.x) * cos(spatial.orientation.y);
        direction = glm::normalize(direction);

        glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::normalize(glm::cross(right, direction));

        spatial.direction = direction;
        spatial.up = up;
        spatial.right = right;
    }
}
