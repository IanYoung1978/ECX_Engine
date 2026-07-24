#include "EC_Frustum.h"

namespace
{
    Plane makePlane(float a, float b, float c, float d)
    {
        Plane plane;
        plane.normal = glm::vec3(a, b, c);
        float len = glm::length(plane.normal);
        if (len > 0.0f)
        {
            plane.normal /= len;
            d /= len;
        }
        plane.d = d;
        return plane;
    }
}

namespace EC_Frustum
{
    std::array<Plane, 6> extractPlanes(const glm::mat4& m)
    {
        std::array<Plane, 6> planes;

        // Left
        planes[0] = makePlane(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]);
        // Right
        planes[1] = makePlane(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]);
        // Bottom
        planes[2] = makePlane(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]);
        // Top
        planes[3] = makePlane(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]);
        // Near
        planes[4] = makePlane(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]);
        // Far
        planes[5] = makePlane(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]);

        return planes;
    }

    bool intersectsAABB(const std::array<Plane, 6>& planes, const AABB& box)
    {
        for (const auto& plane : planes)
        {
            glm::vec3 positiveVertex(
                plane.normal.x >= 0.0f ? box.max.x : box.min.x,
                plane.normal.y >= 0.0f ? box.max.y : box.min.y,
                plane.normal.z >= 0.0f ? box.max.z : box.min.z);

            if (glm::dot(plane.normal, positiveVertex) + plane.d < 0.0f)
                return false;
        }
        return true;
    }
}
