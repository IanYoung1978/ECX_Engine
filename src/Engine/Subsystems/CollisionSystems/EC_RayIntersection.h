#pragma once
#include <glm/glm.hpp>
#include "Components/EC_DOD_Components.h"

// Ray-vs-collider-shape intersection (Issue #30). World-space; `dir` is assumed
// normalized. Sphere/AABB/OBB/Capsule/Cylinder are tested via EC_GJK::raycast against
// each shape's support function - see colliderSupport() below. Plane is the one shape
// GJK can't represent (unbounded, no finite support function in most directions), so it
// keeps an exact closed-form ray-plane test.
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

    // World-space support function (farthest point on the shape's surface in `dir`) for
    // a collider - the shared representation both ray casting (EC_GJK::raycast) and
    // cone containment (EC_GJK::intersects, see EC_BroadPhase::handleConeCheck) are
    // built on, so both tests agree on exactly the same geometry. Builds world-space
    // shape parameters from collider+spatial using the same rotation-matrix-from-spatial
    // approach EC_BroadPhase::computeWorldAABB uses. Plane/Frustum/None have no bounded
    // support and return the collider's world center as a degenerate fallback - callers
    // must not invoke this for those types.
    glm::vec3 colliderSupport(const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial,
        const glm::vec3& dir);

    // Brute-force closest-hit ray-triangle intersection (Möller–Trumbore) against the
    // real geometry in meshData - used for EC_DOD_Collider::Type::Mesh, which (unlike
    // every convex primitive above) has no meaningful single support function: a
    // marching-cubes chunk's surface can be arbitrarily non-convex (curved, overhangs,
    // caves), so this is genuine per-triangle testing, not a GJK approximation. Hit
    // normal is barycentric-interpolated from the mesh's own precomputed per-vertex
    // normals, matching how the surface is actually shaded. A chunk mesh is on the order
    // of a few thousand triangles - fine for occasional ray/cone queries; revisit with a
    // BVH (or, per a flagged future direction, a GPU-based approach) only if profiling
    // ever shows this mattering at higher chunk/triangle counts.
    RayIntersectionResult rayMesh(const glm::vec3& origin, const glm::vec3& dir,
        const EC_DOD_MeshCollisionData& meshData);

    // Dispatches by collider.type: Sphere/AABB/OBB/Capsule/Cylinder go through GJK ray
    // casting against colliderSupport(); Plane uses the closed-form test above; Mesh uses
    // rayMesh() above. Frustum/None are not valid ray targets and return hit = false.
    // meshData is only consulted when collider.type == Mesh - pass a default-constructed
    // one otherwise.
    RayIntersectionResult rayVsCollider(const glm::vec3& origin, const glm::vec3& dir,
        const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial,
        const EC_DOD_MeshCollisionData& meshData = {});
}
