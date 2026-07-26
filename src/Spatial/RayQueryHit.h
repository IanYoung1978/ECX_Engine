#pragma once
#include <glm/glm.hpp>
#include "Entity/EC_DOD_Types.h"

// Shared result type for ray and cone collision queries (Issues #30/#29). Deliberately
// lightweight - just EntityID + glm::vec3/float - so callers like Game.cpp/EC_GameAPI.h
// can use it without pulling in the collision system's broader headers.
//
// For ray-query hits, position/normal describe the actual surface intersection point.
// For cone-query hits, position is the candidate entity's own world position (there is
// no single "surface hit" for a containment/visibility test) and normal is unused
// (left at its default).
struct RayQueryHit
{
    EntityID entity = INVALID_ENTITY;
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f };
    float distance = 0.0f;
};
