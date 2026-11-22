#pragma once
#include <glm/glm.hpp>
#include <functional>

// Generic convex-convex tests via GJK. Each shape is just a support function (farthest point in a
// direction), so one algorithm handles any convex shape, including a cone's curved surface, which
// SAT's explicit axis candidates can't express without approximating it as a pyramid.
namespace EC_GJK
{
    using SupportFn = std::function<glm::vec3(const glm::vec3&)>;

    // Boolean intersection test between two convex shapes (Minkowski-difference GJK).
    bool intersects(const SupportFn& supportA, const SupportFn& supportB);

    // Ray-vs-convex-shape intersection (GJK ray cast, Ericson 4.6): advances a point along the ray using
    // GJK's separating direction, converging exactly for any convex shape. False if the ray misses or
    // the shape lies beyond tmax.
    bool raycast(const glm::vec3& origin, const glm::vec3& dir, float tmax,
        const SupportFn& support, float& outT, glm::vec3& outNormal);
}
