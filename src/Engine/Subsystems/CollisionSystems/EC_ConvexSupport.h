#pragma once
#include <glm/glm.hpp>

// Pure, engine-independent support functions (farthest point on a shape's surface in a
// given world-space direction) for GJK (EC_GJK.h). Deliberately takes only plain glm
// types, no EC_DOD_Collider/EC_DOD_Spatial - keeps this math testable in isolation
// (see tests/EC_GJK_Tests.cpp) without pulling in the ECS/engine header stack.
// EC_RayIntersection::colliderSupport() is the thin adapter that unpacks a collider
// component into these plain parameters.
namespace EC_ConvexSupport
{
    glm::vec3 sphereSupport(const glm::vec3& center, float radius, const glm::vec3& dir);

    // orientation must be orthonormal (its inverse is assumed to be its transpose).
    glm::vec3 boxSupport(const glm::vec3& center, const glm::vec3& halfExtents,
        const glm::mat3& orientation, const glm::vec3& dir);

    glm::vec3 capsuleSupport(const glm::vec3& pointA, const glm::vec3& pointB, float radius,
        const glm::vec3& dir);

    glm::vec3 cylinderSupport(const glm::vec3& pointA, const glm::vec3& pointB, float radius,
        const glm::vec3& dir);

    // Support function for a finite right circular cone: the convex hull of the apex
    // point and a base disk of radius `height * tan(halfAngleRadians)` centered at
    // `apex + axis * height`. `axis` must be a unit vector.
    glm::vec3 coneSupport(const glm::vec3& apex, const glm::vec3& axis, float halfAngleRadians,
        float height, const glm::vec3& dir);
}
