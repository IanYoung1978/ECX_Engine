#pragma once
#include <glm/glm.hpp>
#include "Entity/EC_DOD_Types.h"

// Shared result type for ray and cone queries, kept lightweight so callers don't pull in the
// collision headers. For ray hits, position/normal describe the surface intersection; for cone
// hits, position is the entity's world position and normal is unused.
struct RayQueryHit
{
    EntityID entity = INVALID_ENTITY;
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f };
    float distance = 0.0f;
};
