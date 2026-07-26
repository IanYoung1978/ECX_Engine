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
            return worldCenter; // Plane/Frustum/None - not a valid GJK target, see rayVsCollider
        }
    }

    RayIntersectionResult rayVsCollider(const glm::vec3& origin, const glm::vec3& dir,
        const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial)
    {
        RayIntersectionResult result;

        if (collider.type == EC_DOD_Collider::Type::Plane)
        {
            glm::vec3 up = glm::normalize(spatial.up);
            glm::vec3 worldCenter = spatial.position + collider.center;
            return rayPlane(origin, dir, up, glm::dot(up, worldCenter));
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
