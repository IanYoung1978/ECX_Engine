#include "EC_CollisionChecks.h"

#include <algorithm>
#include <cmath>
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
    const glm::vec3 contact = (overlapMin + overlapMax) * 0.5f;

    manifold.contactNormal = normal;
    manifold.penetrationDepth = penetration;
    manifold.contactPoints = { contact };
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

    // Test axes L = A0, A1, A2
    for (int i = 0; i < 3; i++) {
        ra = obbA.halfExtents[i];
        rb = obbB.halfExtents[0] * absR[i][0] + obbB.halfExtents[1] * absR[i][1] + obbB.halfExtents[2] * absR[i][2];
        if (glm::abs(t[i]) > ra + rb) {
            return false;
        }
    }

    // Test axes L = B0, B1, B2
    for (int i = 0; i < 3; i++) {
        ra = obbA.halfExtents[0] * absR[0][i] + obbA.halfExtents[1] * absR[1][i] + obbA.halfExtents[2] * absR[2][i];
        rb = obbB.halfExtents[i];
        if (glm::abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) {
            return false;
        }
    }

    // Test axis L = A0 x B0
    ra = obbA.halfExtents[1] * absR[2][0] + obbA.halfExtents[2] * absR[1][0];
    rb = obbB.halfExtents[1] * absR[0][2] + obbB.halfExtents[2] * absR[0][1];
    if (glm::abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) {
        return false;
    }

    // Test axis L = A0 x B1
    ra = obbA.halfExtents[1] * absR[2][1] + obbA.halfExtents[2] * absR[1][1];
    rb = obbB.halfExtents[0] * absR[0][2] + obbB.halfExtents[2] * absR[0][0];
    if (glm::abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) {
        return false;
    }

    // Test axis L = A0 x B2
    ra = obbA.halfExtents[1] * absR[2][2] + obbA.halfExtents[2] * absR[1][2];
    rb = obbB.halfExtents[0] * absR[0][1] + obbB.halfExtents[1] * absR[0][0];
    if (glm::abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) {
        return false;
    }

    // Test axis L = A1 x B0
    ra = obbA.halfExtents[0] * absR[2][0] + obbA.halfExtents[2] * absR[0][0];
    rb = obbB.halfExtents[1] * absR[1][2] + obbB.halfExtents[2] * absR[1][1];
    if (glm::abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) {
        return false;
    }

    // Test axis L = A1 x B1
    ra = obbA.halfExtents[0] * absR[2][1] + obbA.halfExtents[2] * absR[0][1];
    rb = obbB.halfExtents[0] * absR[1][2] + obbB.halfExtents[2] * absR[1][0];
    if (glm::abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) {
        return false;
    }

    // Test axis L = A1 x B2
    ra = obbA.halfExtents[0] * absR[2][2] + obbA.halfExtents[2] * absR[0][2];
    rb = obbB.halfExtents[0] * absR[1][1] + obbB.halfExtents[1] * absR[1][0];
    if (glm::abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) {
        return false;
    }

    // Test axis L = A2 x B0
    ra = obbA.halfExtents[0] * absR[1][0] + obbA.halfExtents[1] * absR[0][0];
    rb = obbB.halfExtents[1] * absR[2][2] + obbB.halfExtents[2] * absR[2][1];
    if (glm::abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) {
        return false;
    }

    // Test axis L = A2 x B1
    ra = obbA.halfExtents[0] * absR[1][1] + obbA.halfExtents[1] * absR[0][1];
    rb = obbB.halfExtents[0] * absR[2][2] + obbB.halfExtents[2] * absR[2][0];
    if (glm::abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) {
        return false;
    }

    // Test axis L = A2 x B2
    ra = obbA.halfExtents[0] * absR[1][2] + obbA.halfExtents[1] * absR[0][2];
    rb = obbB.halfExtents[0] * absR[2][1] + obbB.halfExtents[1] * absR[2][0];
    if (glm::abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) {
        return false;
    }

    // No separating axis found - OBBs are colliding

    manifold.contactNormal = axisA[0];
    manifold.contactPoints.push_back((centerA + centerB) * 0.5f);
    manifold.penetrationDepth = 0.1f;

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
