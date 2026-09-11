#include "EC_RayIntersection.h"
#include "EC_GJK.h"
#include "EC_ConvexSupport.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace EC_RayIntersection
{
    RayIntersectionResult rayPlane(const glm::vec3& origin, const glm::vec3& dir,
        const glm::vec3& normal, float d)
    {
        RayIntersectionResult result;

        float denom = glm::dot(normal, dir);
        if (std::abs(denom) < 1e-8f)
            return result;

        float t = (d - glm::dot(normal, origin)) / denom;
        if (t < 0.0f)
            return result;

        result.hit = true;
        result.distance = t;
        result.position = origin + dir * t;
        result.normal = (denom < 0.0f) ? normal : -normal;
        return result;
    }

    glm::vec3 colliderSupport(const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial,
        const glm::vec3& dir)
    {
        glm::vec3 worldCenter = spatial.position + collider.center;
        glm::vec3 forward = glm::normalize(spatial.direction);
        glm::vec3 right = glm::normalize(spatial.right);
        glm::vec3 up = glm::normalize(spatial.up);
        glm::mat3 rotation(right, up, forward);

        switch (collider.type)
        {
        case EC_DOD_Collider::Type::Sphere:
            return EC_ConvexSupport::sphereSupport(worldCenter, collider.radius, dir);

        case EC_DOD_Collider::Type::AABB:
            return EC_ConvexSupport::boxSupport(worldCenter, collider.extents, glm::mat3(1.0f), dir);

        case EC_DOD_Collider::Type::OBB:
            return EC_ConvexSupport::boxSupport(worldCenter, collider.extents, rotation, dir);

        case EC_DOD_Collider::Type::Capsule:
        {
            glm::vec3 halfHeight = up * (collider.height * 0.5f);
            return EC_ConvexSupport::capsuleSupport(worldCenter - halfHeight, worldCenter + halfHeight, collider.radius, dir);
        }

        case EC_DOD_Collider::Type::Cylinder:
        {
            glm::vec3 halfHeight = up * (collider.height * 0.5f);
            return EC_ConvexSupport::cylinderSupport(worldCenter - halfHeight, worldCenter + halfHeight, collider.radius, dir);
        }

        default:
            return worldCenter; // Plane/Frustum/Mesh/None - not a valid GJK target, see rayVsCollider
        }
    }

    // Standard Möller–Trumbore ray-triangle test, returning t (distance along dir) on a
    // hit, or a negative value on miss. Backface-tolerant (doesn't cull based on winding)
    // since a chunk's marching-tetrahedra output isn't guaranteed consistently wound from
    // every angle a query might come from.
    namespace {
        constexpr float kEpsilon = 1e-7f;

        bool rayTriangle(const glm::vec3& origin, const glm::vec3& dir,
            const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
            float& outT, float& outU, float& outV)
        {
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 pvec = glm::cross(dir, edge2);
            float det = glm::dot(edge1, pvec);
            if (std::abs(det) < kEpsilon)
                return false; // ray parallel to triangle plane

            float invDet = 1.0f / det;
            glm::vec3 tvec = origin - v0;
            outU = glm::dot(tvec, pvec) * invDet;
            if (outU < 0.0f || outU > 1.0f)
                return false;

            glm::vec3 qvec = glm::cross(tvec, edge1);
            outV = glm::dot(dir, qvec) * invDet;
            if (outV < 0.0f || outU + outV > 1.0f)
                return false;

            outT = glm::dot(edge2, qvec) * invDet;
            return outT >= 0.0f;
        }
    }

    RayIntersectionResult rayMesh(const glm::vec3& origin, const glm::vec3& dir,
        const EC_DOD_MeshCollisionData& meshData)
    {
        RayIntersectionResult result;
        if (!meshData.positions || !meshData.indices || !meshData.normals)
            return result;

        const auto& positions = *meshData.positions;
        const auto& normals = *meshData.normals;
        const auto& indices = *meshData.indices;

        float closestT = std::numeric_limits<float>::max();
        bool found = false;
        float bestU = 0.0f, bestV = 0.0f;
        uint32_t bestI0 = 0, bestI1 = 0, bestI2 = 0;

        for (size_t i = 0; i + 2 < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
            float t, u, v;
            if (rayTriangle(origin, dir, positions[i0], positions[i1], positions[i2], t, u, v)
                && t < closestT)
            {
                closestT = t;
                bestU = u;
                bestV = v;
                bestI0 = i0; bestI1 = i1; bestI2 = i2;
                found = true;
            }
        }

        if (!found)
            return result;

        result.hit = true;
        result.distance = closestT;
        result.position = origin + dir * closestT;
        // Barycentric interpolation: (1-u-v)*n0 + u*n1 + v*n2, matching rayTriangle's
        // u/v convention (weights for v1/v2 respectively, remainder for v0).
        result.normal = glm::normalize(
            (1.0f - bestU - bestV) * normals[bestI0] + bestU * normals[bestI1] + bestV * normals[bestI2]);
        return result;
    }

    RayIntersectionResult rayVsCollider(const glm::vec3& origin, const glm::vec3& dir,
        const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial,
        const EC_DOD_MeshCollisionData& meshData)
    {
        RayIntersectionResult result;

        if (collider.type == EC_DOD_Collider::Type::Plane)
        {
            glm::vec3 up = glm::normalize(spatial.up);
            glm::vec3 worldCenter = spatial.position + collider.center;
            return rayPlane(origin, dir, up, glm::dot(up, worldCenter));
        }

        if (collider.type == EC_DOD_Collider::Type::Mesh)
        {
            // EC_DOD_MeshCollisionData's positions are chunk-local (0..kChunkWorldSize),
            // matching EC_MarchingCubesMesher's own local-grid-index output - not world
            // space (rendering places them via the entity's transform, same as any other
            // local-space mesh + world placement). Test in that same local space by
            // offsetting the ray, then translate the hit back to world space - direction
            // needs no adjustment (chunks are never rotated, and a direction vector is
            // translation-invariant regardless).
            RayIntersectionResult meshResult = rayMesh(origin - spatial.position, dir, meshData);
            if (meshResult.hit)
                meshResult.position += spatial.position;
            return meshResult;
        }

        if (collider.type == EC_DOD_Collider::Type::Frustum || collider.type == EC_DOD_Collider::Type::None)
            return result;

        EC_GJK::SupportFn support = [&collider, &spatial](const glm::vec3& d) {
            return colliderSupport(collider, spatial, d);
        };

        float t = 0.0f;
        glm::vec3 normal(0.0f);
        // Unbounded here deliberately - EC_BroadPhase::castRay clips against its own
        // maxDistance by discarding hits whose distance exceeds it.
        if (!EC_GJK::raycast(origin, dir, std::numeric_limits<float>::max(), support, t, normal))
            return result;

        result.hit = true;
        result.distance = t;
        result.position = origin + dir * t;
        result.normal = normal;
        return result;
    }
}
