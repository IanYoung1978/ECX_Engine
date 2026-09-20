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

// Position integration (position += velocity * dt) happens here for every entity with a
// Spatial, RigidBody included - it's the same correct vector addition regardless of what
// else is going on with the entity, so there's exactly one place it happens, per the
// engine's own pipeline order (Spatial moves things; Collision/Physics run after).
//
// Orientation is different: EC_PhysicsSystem's own integration step owns it via a real
// quaternion rotation rather than this system's simpler Euler-angle accumulation below (see
// that function's own comment - Euler accumulation only tracks rotation correctly about one
// fixed axis at a time, which a tumbling rigid body needs to not be true of) - but only for
// a Dynamic body (see EC_BodyType's own comment). A Kinematic RigidBody, used purely for
// gravity bookkeeping on an otherwise script/mouse-controlled entity (the "arcade physics"
// pattern), never has anything driving angVelocity, so skipping its orientation here would
// silently strand any direct setOrientation()/mouse-look call with nothing left to turn it
// into a new facing direction - exactly the bug this comment used to cause.
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
