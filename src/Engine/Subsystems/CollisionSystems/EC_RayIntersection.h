#pragma once
#include <glm/glm.hpp>
#include "Components/EC_DOD_Components.h"

// Ray-vs-collider intersection in world space; `dir` must be normalized. Sphere/AABB/OBB/Capsule/
// Cylinder go through EC_GJK::raycast on each shape's support function (see colliderSupport());
// Plane has no finite support function, so it keeps an exact closed-form test.
struct RayIntersectionResult
{
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 position{ 0.0f };
    glm::vec3 normal{ 0.0f };
};

namespace EC_RayIntersection
{
    RayIntersectionResult rayPlane(const glm::vec3& origin, const glm::vec3& dir,
        const glm::vec3& normal, float d);

    // World-space support function for a collider, shared by ray casting (EC_GJK::raycast) and cone
    // containment (EC_GJK::intersects) so both agree on the geometry. Plane/Frustum/None have no bounded
    // support and return the collider's world centre; callers must not call this for those types.
    glm::vec3 colliderSupport(const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial,
        const glm::vec3& dir);

    // Brute-force closest-hit Moller-Trumbore against meshData's real triangles (Mesh colliders can be
    // non-convex, so no GJK). The hit normal is interpolated from the mesh's per-vertex normals.
    RayIntersectionResult rayMesh(const glm::vec3& origin, const glm::vec3& dir,
        const EC_DOD_MeshCollisionData& meshData);

    // Dispatches by collider.type: Sphere/AABB/OBB/Capsule/Cylinder via GJK ray casting, Plane
    // closed-form, Mesh via rayMesh(). Frustum/None return hit = false. meshData is only used for Mesh;
    // pass a default-constructed one otherwise.
    RayIntersectionResult rayVsCollider(const glm::vec3& origin, const glm::vec3& dir,
        const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial,
        const EC_DOD_MeshCollisionData& meshData = {});
}
