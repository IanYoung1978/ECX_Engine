#include "Engine/Subsystems/EC_SpatialSystem.h"
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

// Handles simple, non-physics spatial movement only (camera, script-driven
// entities via entity:setPosition/moveForward etc.) - anything with an
// EC_DOD_RigidBody is skipped entirely and is fully owned by the two-stage
// rigid-body pipeline instead: EC_PhysicsResolution (collision resolution,
// caches impulses) followed by EC_PhysicsSystem (gravity + damping +
// consuming those cached impulses + integration), which runs later this
// same tick. Handling a RigidBody entity here too would double-integrate it.
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

        EntityID entity = spatialArray->getEntity(i);
        if (manager.hasComponent<EC_DOD_RigidBody>(entity)) {
            continue;
        }

        spatial.position += spatial.velocity * deltaTimeS;
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
