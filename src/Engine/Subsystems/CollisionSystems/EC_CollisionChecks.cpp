#include "EC_CollisionChecks.h"
#include "EC_VClip.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

namespace {
    constexpr float kEpsilon = 1e-6f;

    glm::vec3 safeNormalize(const glm::vec3& v, const glm::vec3& fallback = glm::vec3(0.0f, 1.0f, 0.0f)) {
        const float len2 = glm::dot(v, v);
        if (len2 <= kEpsilon) {
            return fallback;
        }
        return v / std::sqrt(len2);
    }

    glm::vec3 clampVec3(const glm::vec3& v, const glm::vec3& minV, const glm::vec3& maxV) {
        return glm::vec3(
            std::clamp(v.x, minV.x, maxV.x),
            std::clamp(v.y, minV.y, maxV.y),
            std::clamp(v.z, minV.z, maxV.z));
    }

    AABB toWorldAABB(const AABB& aabb, const glm::vec3& pos) {
        return { aabb.min + pos, aabb.max + pos };
    }
}


bool EC_CollisionChecks::SphereVsSphere(const Sphere& sphereA, const glm::vec3& posA, const Sphere& sphereB, const glm::vec3& posB, CollisionManifold& manifold)
{
    const glm::vec3 centerA = posA + sphereA.center;
    const glm::vec3 centerB = posB + sphereB.center;
    const glm::vec3 delta = centerB - centerA;
    const float dist2 = glm::dot(delta, delta);
    const float radiusSum = sphereA.radius + sphereB.radius;

    if (dist2 > radiusSum * radiusSum) {
        return false;
    }

    const float distance = std::sqrt(std::max(dist2, 0.0f));
    const glm::vec3 normal = (distance > kEpsilon) ? (delta / distance) : glm::vec3(1.0f, 0.0f, 0.0f);
    const float penetration = radiusSum - distance;
    const glm::vec3 contact = centerA + normal * (sphereA.radius - penetration * 0.5f);

    manifold.contactNormal = normal;
    manifold.penetrationDepth = penetration;
    manifold.contactPoints = { contact };
    return true;

}

bool EC_CollisionChecks::AABBVsAABB(const AABB& aabbA, const glm::vec3& posA, const AABB& aabbB, const glm::vec3& posB, CollisionManifold& manifold)
{
    const AABB wa = toWorldAABB(aabbA, posA);
    const AABB wb = toWorldAABB(aabbB, posB);

    if (wa.max.x < wb.min.x || wa.min.x > wb.max.x ||
        wa.max.y < wb.min.y || wa.min.y > wb.max.y ||
        wa.max.z < wb.min.z || wa.min.z > wb.max.z) {
        return false;
    }

    const float overlapX = std::min(wa.max.x, wb.max.x) - std::max(wa.min.x, wb.min.x);
    const float overlapY = std::min(wa.max.y, wb.max.y) - std::max(wa.min.y, wb.min.y);
    const float overlapZ = std::min(wa.max.z, wb.max.z) - std::max(wa.min.z, wb.min.z);

    glm::vec3 normal(1.0f, 0.0f, 0.0f);
    float penetration = overlapX;

    const glm::vec3 centerA = (wa.min + wa.max) * 0.5f;
    const glm::vec3 centerB = (wb.min + wb.max) * 0.5f;

    if (overlapY < penetration) {
        penetration = overlapY;
        normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if (overlapZ < penetration) {
        penetration = overlapZ;
        normal = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    const glm::vec3 delta = centerB - centerA;
    if (glm::dot(delta, normal) < 0.0f) {
        normal = -normal;
    }

    const glm::vec3 overlapMin = glm::max(wa.min, wb.min);
    const glm::vec3 overlapMax = glm::min(wa.max, wb.max);

    // Contact face: the 4 corners of the overlap region in the plane
    // perpendicular to the collision normal (both boxes are axis-aligned, so
    // this is exact - no clipping needed, unlike the general OBB case).
    // Multiple points let a resting/toppled box get counter-torque from all
    // 4 corners at once, instead of pivoting/rolling around one.
    manifold.contactNormal = normal;
    manifold.penetrationDepth = penetration;
    manifold.contactPoints.clear();

    if (normal.y != 0.0f) {
        const float y = (overlapMin.y + overlapMax.y) * 0.5f;
        manifold.contactPoints = {
            { overlapMin.x, y, overlapMin.z },
            { overlapMax.x, y, overlapMin.z },
            { overlapMax.x, y, overlapMax.z },
            { overlapMin.x, y, overlapMax.z },
        };
    }
    else if (normal.x != 0.0f) {
        const float x = (overlapMin.x + overlapMax.x) * 0.5f;
        manifold.contactPoints = {
            { x, overlapMin.y, overlapMin.z },
            { x, overlapMax.y, overlapMin.z },
            { x, overlapMax.y, overlapMax.z },
            { x, overlapMin.y, overlapMax.z },
        };
    }
    else {
        const float z = (overlapMin.z + overlapMax.z) * 0.5f;
        manifold.contactPoints = {
            { overlapMin.x, overlapMin.y, z },
            { overlapMax.x, overlapMin.y, z },
            { overlapMax.x, overlapMax.y, z },
            { overlapMin.x, overlapMax.y, z },
        };
    }

    return true;

}
bool EC_CollisionChecks::OBBVsOBB(const OBB& obbA, const glm::vec3& posA,
    const OBB& obbB, const glm::vec3& posB,
    CollisionManifold& manifold)
{
    glm::vec3 centerA = posA + obbA.center;
    glm::vec3 centerB = posB + obbB.center;

    // Get the 3 axes of each OBB
    glm::vec3 axisA[3] = {
        obbA.orientation[0],
        obbA.orientation[1],
        obbA.orientation[2]
    };

    glm::vec3 axisB[3] = {
        obbB.orientation[0],
        obbB.orientation[1],
        obbB.orientation[2]
    };

    // Vector between centers
    glm::vec3 T = centerB - centerA;

    float ra, rb;
    glm::mat3 R, absR;

    // Compute rotation matrix expressing B in A's coordinate frame
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = glm::dot(axisA[i], axisB[j]);
        }
    }

    // Compute translation vector T in A's frame
    glm::vec3 t(glm::dot(T, axisA[0]), glm::dot(T, axisA[1]), glm::dot(T, axisA[2]));

    // Compute absolute values with epsilon for numerical stability
    const float epsilon = 0.00001f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            absR[i][j] = glm::abs(R[i][j]) + epsilon;
        }
    }

    // Track the axis with the smallest penetration across all 15 candidate
    // axes (standard SAT-derived manifold technique) - that axis becomes the
    // contact normal. Unlike the boolean-only test, cross-product (edge-edge)
    // axes must be normalized here so their overlap is a real distance,
    // comparable against the face axes.
    float bestOverlap = 1e30f;
    glm::vec3 bestAxis(0.0f);

    // Test axes L = A0, A1, A2
    for (int i = 0; i < 3; i++) {
        ra = obbA.halfExtents[i];
        rb = obbB.halfExtents[0] * absR[i][0] + obbB.halfExtents[1] * absR[i][1] + obbB.halfExtents[2] * absR[i][2];
        float overlap = ra + rb - glm::abs(t[i]);
        if (overlap < 0.0f) return false;
        if (overlap < bestOverlap) { bestOverlap = overlap; bestAxis = axisA[i]; }
    }

    // Test axes L = B0, B1, B2
    for (int i = 0; i < 3; i++) {
        ra = obbA.halfExtents[0] * absR[0][i] + obbA.halfExtents[1] * absR[1][i] + obbA.halfExtents[2] * absR[2][i];
        rb = obbB.halfExtents[i];
        float proj = t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i];
        float overlap = ra + rb - glm::abs(proj);
        if (overlap < 0.0f) return false;
        if (overlap < bestOverlap) { bestOverlap = overlap; bestAxis = axisB[i]; }
    }

    // Test the 9 edge-edge cross-product axes L = Ai x Bj. For axis-aligned
    // (or near-axis-aligned) boxes - e.g. a cube resting flat on the floor -
    // several of these 9 axes are mathematically identical to a face axis
    // already tested above (cross(X,Y) is the same line as the Z face axis),
    // just computed through a different formula path with different
    // floating-point rounding. Without a tie-breaking bias, that rounding
    // noise can make the "duplicate" edge-edge overlap come out marginally
    // smaller than the true face overlap and win the naive minimum
    // comparison - which permanently misclassifies a flat face-face rest as
    // a single-point edge-edge contact, and a box resting on one point
    // instead of a stable 4-point face can only pivot/rotate freely about
    // its own centre rather than being held flat. kEdgeEdgeBias requires an
    // edge-edge axis to beat the best face overlap by a real margin, not
    // just numerically, before it's allowed to override a face axis -
    // standard SAT practice for exactly this degeneracy. Kept deliberately
    // small: the true floating-point noise between the duplicate axis-
    // aligned computations is only ~1e-5 to 1e-4 at this engine's unit
    // scale, so this only needs to be comfortably above that - not so
    // large that it starts overriding a GENUINE edge/vertex contact for a
    // meaningfully tilted box (e.g. a corner striking the floor mid-topple)
    // in favour of an incorrect face axis, which would understate that
    // corner's true penetration depth and let it sink before the impulse
    // solver reacts properly.
    constexpr float kEdgeEdgeBias = 0.001f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 axis = glm::cross(axisA[i], axisB[j]);
            float len2 = glm::dot(axis, axis);
            if (len2 < 1e-8f) continue; // near-parallel edges - degenerate, skip

            const float invLen = 1.0f / std::sqrt(len2);
            axis *= invLen;

            float raGen = 0.0f, rbGen = 0.0f;
            for (int k = 0; k < 3; k++) {
                raGen += obbA.halfExtents[k] * glm::abs(glm::dot(axisA[k], axis));
                rbGen += obbB.halfExtents[k] * glm::abs(glm::dot(axisB[k], axis));
            }
            float overlap = raGen + rbGen - glm::abs(glm::dot(T, axis));
            if (overlap < 0.0f) return false;
            if (overlap + kEdgeEdgeBias < bestOverlap) {
                bestOverlap = overlap; bestAxis = axis;
            }
        }
    }

    // No separating axis found - OBBs are colliding. bestAxis/bestOverlap now
    // hold the minimum-penetration axis and depth.
    glm::vec3 normal = bestAxis;
    if (glm::dot(T, normal) < 0.0f) normal = -normal; // orient A -> B

    manifold.contactNormal = normal;
    manifold.penetrationDepth = bestOverlap;

    // SAT above is only used for the boolean test and penetration depth -
    // both robust regardless of body size. Which FEATURE pair (face/edge/
    // vertex on each box) is actually touching is picked independently here,
    // by walking to the true closest features via local Voronoi-region
    // classification rather than SAT's raw numeric overlap comparison, which
    // misclassifies "edge-edge" whenever body sizes are grossly mismatched
    // (e.g. a cube vs a room-sized floor slab) - see EC_VClip.h for why.
    EC_VClip::generateContactPoints(obbA, posA, obbB, posB, normal, bestOverlap, manifold);

    return true;
}

bool EC_CollisionChecks::SphereVsAABB(const Sphere& sphere, const glm::vec3& spherePos, const AABB& aabb, const glm::vec3& aabbPos, CollisionManifold& manifold)
{
    const glm::vec3 center = spherePos + sphere.center;
    const AABB waabb = toWorldAABB(aabb, aabbPos);

    const glm::vec3 closest = clampVec3(center, waabb.min, waabb.max);
    const glm::vec3 delta = center - closest;
    const float dist2 = glm::dot(delta, delta);

    if (dist2 > sphere.radius * sphere.radius) {
        return false;
    }

    const float dist = std::sqrt(std::max(dist2, 0.0f));
    const glm::vec3 normal = (dist > kEpsilon) ? (delta / dist) : glm::vec3(0.0f, 1.0f, 0.0f);

    manifold.contactNormal = normal;
    manifold.penetrationDepth = sphere.radius - dist;
    manifold.contactPoints = { closest };
    return true;

}

bool EC_CollisionChecks::SphereVsOBB(const Sphere& sphere, const glm::vec3& spherePos, const OBB& obb, const glm::vec3& obbPos, CollisionManifold& manifold)
{
    const glm::vec3 worldCenter = spherePos + sphere.center;
    const glm::vec3 obbCenter = obbPos + obb.center;

    const glm::mat3 orientation = obb.orientation;
    const glm::mat3 invOrientation = glm::transpose(orientation);

    const glm::vec3 localSphere = invOrientation * (worldCenter - obbCenter);
    const glm::vec3 localClosest = clampVec3(localSphere, -obb.halfExtents, obb.halfExtents);
    const glm::vec3 localDelta = localSphere - localClosest;
    const float dist2 = glm::dot(localDelta, localDelta);

    if (dist2 > sphere.radius * sphere.radius) {
        return false;
    }

    const glm::vec3 worldClosest = obbCenter + orientation * localClosest;
    const glm::vec3 worldDelta = worldCenter - worldClosest;
    const float dist = std::sqrt(std::max(glm::dot(worldDelta, worldDelta), 0.0f));

    manifold.contactNormal = safeNormalize(worldDelta);
    manifold.penetrationDepth = sphere.radius - dist;
    manifold.contactPoints = { worldClosest };
    return true;

}

bool EC_CollisionChecks::FrustumVsAABB(const Frustum& frustum, const glm::vec3& frustumPos, const AABB& aabb, const glm::vec3& aabbPos)
{
    // Conservative approximation for now: frustum bound as sphere.
    const float halfFovRad = glm::radians(frustum.fov * 0.5f);
    const float frustumRadius = std::max(frustum.farPlane * std::tan(halfFovRad), frustum.farPlane);

    const glm::vec3 frustumCenter = frustumPos + frustum.direction * (frustum.farPlane * 0.5f);
    const glm::vec3 boxCenter = ((aabb.min + aabb.max) * 0.5f) + aabbPos;
    const glm::vec3 boxExtents = (aabb.max - aabb.min) * 0.5f;

    const glm::vec3 delta = frustumCenter - boxCenter;
    const glm::vec3 closest = clampVec3(delta, -boxExtents, boxExtents);
    const glm::vec3 diff = delta - closest;
    return glm::dot(diff, diff) <= frustumRadius * frustumRadius;

}
