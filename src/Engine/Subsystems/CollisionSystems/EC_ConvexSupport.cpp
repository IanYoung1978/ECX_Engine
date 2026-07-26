#include "EC_ConvexSupport.h"
#include <cmath>

namespace EC_ConvexSupport
{
    glm::vec3 sphereSupport(const glm::vec3& center, float radius, const glm::vec3& dir)
    {
        float len = glm::length(dir);
        glm::vec3 n = (len > 1e-8f) ? (dir / len) : glm::vec3(0.0f, 1.0f, 0.0f);
        return center + n * radius;
    }

    glm::vec3 boxSupport(const glm::vec3& center, const glm::vec3& halfExtents,
        const glm::mat3& orientation, const glm::vec3& dir)
    {
        glm::vec3 localDir = glm::transpose(orientation) * dir;
        glm::vec3 localPoint(
            (localDir.x >= 0.0f) ? halfExtents.x : -halfExtents.x,
            (localDir.y >= 0.0f) ? halfExtents.y : -halfExtents.y,
            (localDir.z >= 0.0f) ? halfExtents.z : -halfExtents.z);
        return center + orientation * localPoint;
    }

    glm::vec3 capsuleSupport(const glm::vec3& pointA, const glm::vec3& pointB, float radius,
        const glm::vec3& dir)
    {
        glm::vec3 base = (glm::dot(dir, pointA) > glm::dot(dir, pointB)) ? pointA : pointB;
        float len = glm::length(dir);
        glm::vec3 n = (len > 1e-8f) ? (dir / len) : glm::vec3(0.0f, 1.0f, 0.0f);
        return base + n * radius;
    }

    glm::vec3 cylinderSupport(const glm::vec3& pointA, const glm::vec3& pointB, float radius,
        const glm::vec3& dir)
    {
        glm::vec3 axis = glm::normalize(pointB - pointA);
        glm::vec3 capCenter = (glm::dot(dir, axis) >= 0.0f) ? pointB : pointA;
        glm::vec3 radial = dir - axis * glm::dot(dir, axis);
        float radialLen = glm::length(radial);
        glm::vec3 offset = (radialLen > 1e-8f) ? (radial / radialLen) * radius : glm::vec3(0.0f);
        return capCenter + offset;
    }

    glm::vec3 coneSupport(const glm::vec3& apex, const glm::vec3& axis, float halfAngleRadians,
        float height, const glm::vec3& dir)
    {
        glm::vec3 baseCenter = apex + axis * height;
        float baseRadius = height * std::tan(halfAngleRadians);

        glm::vec3 dirOnPlane = dir - axis * glm::dot(dir, axis);
        float planeLen = glm::length(dirOnPlane);
        glm::vec3 baseSupport = (planeLen > 1e-6f)
            ? baseCenter + (dirOnPlane / planeLen) * baseRadius
            : baseCenter;

        return (glm::dot(dir, apex) > glm::dot(dir, baseSupport)) ? apex : baseSupport;
    }
}
