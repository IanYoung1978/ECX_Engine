#include "Engine/Subsystems/EC_TransformSystem.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

EC_TransformSystem::EC_TransformSystem() {}
EC_TransformSystem::~EC_TransformSystem() {}

void EC_TransformSystem::init(ECXMessenger& messenger, EC_Game& game) {}

static glm::mat4 buildLocal(const EC_DOD_Spatial& spatial, const EC_DOD_Transform& transform) {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), spatial.position);
    glm::mat4 r = glm::mat4(1.0f);
    r = glm::rotate(r, spatial.orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    r = glm::rotate(r, spatial.orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    r = glm::rotate(r, spatial.orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 s = glm::scale(glm::mat4(1.0f), transform.scale);
    return t * r * s;
}

void EC_TransformSystem::update(const float& deltaTimeS, EC_Game& game) {
    auto& manager = EC_DOD_EntityManager::getInstance();

    auto entities = manager.getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Spatial)),
        std::type_index(typeid(EC_DOD_Transform))
        });

    // Sort by depth so parents are always processed before children
    std::sort(entities.begin(), entities.end(), [&manager](EntityID a, EntityID b) {
        uint32_t depthA = 0, depthB = 0;
        if (manager.hasComponent<EC_DOD_Hierarchy>(a))
            depthA = manager.getComponent<EC_DOD_Hierarchy>(a).depth;
        if (manager.hasComponent<EC_DOD_Hierarchy>(b))
            depthB = manager.getComponent<EC_DOD_Hierarchy>(b).depth;
        return depthA < depthB;
        });

    // Single forward pass
    for (EntityID entity : entities) {
        const auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);
        auto& transform = manager.getComponent<EC_DOD_Transform>(entity);

        glm::mat4 local = buildLocal(spatial, transform);

        if (manager.hasComponent<EC_DOD_Hierarchy>(entity)) {
            const auto& hierarchy = manager.getComponent<EC_DOD_Hierarchy>(entity);
            if (hierarchy.parent != INVALID_ENTITY &&
                manager.hasComponent<EC_DOD_Transform>(hierarchy.parent)) {
                const auto& parentTransform = manager.getComponent<EC_DOD_Transform>(hierarchy.parent);
                transform.matrix = parentTransform.matrix * local;
            }
            else {
                transform.matrix = local;
            }
        }
        else {
            transform.matrix = local;
        }

        transform.dirty = false;
    }
}