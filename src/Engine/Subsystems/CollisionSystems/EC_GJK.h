#pragma once
#include <glm/glm.hpp>
#include <functional>

// Generic convex-convex tests via GJK (Gilbert-Johnson-Keerthi). Each shape is
// represented purely by its support function - the farthest point on the shape's
// surface in a given world-space direction - so the same algorithms handle any convex
// shape, curved or polytope, without shape-pair-specific code. This is why GJK rather
// than SAT: SAT needs an explicit set of separating-axis candidates per shape pair
// (face normals, edge cross products), which doesn't extend to a cone's curved lateral
// surface without approximating it as a many-sided pyramid first.
namespace EC_GJK
{
    using SupportFn = std::function<glm::vec3(const glm::vec3&)>;

    // Boolean intersection test between two convex shapes (Minkowski-difference GJK).
    bool intersects(const SupportFn& supportA, const SupportFn& supportB);

    // Ray-vs-convex-shape intersection (Gino van den Bergen's GJK ray cast /
    // conservative advancement algorithm - Ericson, "Real-Time Collision Detection",
    // 4.6). Advances a point along the ray toward the shape using the separating
    // direction found by GJK's point-to-shape distance query at each step; converges
    // exactly to the true intersection distance for any convex shape. Returns false if
    // the ray misses or the shape lies beyond tmax.
    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float tmax,
        const SupportFn& support, float& outT, glm::vec3& outNormal);
}
