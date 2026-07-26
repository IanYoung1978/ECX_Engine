#pragma once
#include <glm/glm.hpp>
#include <array>
#include "Engine/Subsystems/CollisionSystems/EC_CollisionShapes.h"

// Frustum-plane extraction and testing. No frustum-plane infrastructure existed
// anywhere in the engine before this - cameras only ever built a view matrix.
namespace EC_Frustum
{
    // Extracts the 6 frustum planes (left, right, bottom, top, near, far) from a
    // combined view*projection matrix (Gribb-Hartmann method). Plane normals point
    // inward, toward the frustum interior.
    std::array<Plane, 6> extractPlanes(const glm::mat4& viewProjection);

    // Fast conservative AABB-vs-frustum test: false only if the box is fully outside
    // at least one plane. May return true for some boxes that are fully outside
    // (no false negatives, occasional false positives) - appropriate for cell-level
    // culling where cheap-and-safe beats exact.
    bool intersectsAABB(const std::array<Plane, 6>& planes, const AABB& box);
}
