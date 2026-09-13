#include "Engine/Subsystems/Scripting/EC_VolumeAPI.h"

namespace ScriptAPI
{
    VolumeHandle VolumeAPI::constant(float value) { return { EC_Volume::constant(value) }; }

    VolumeHandle VolumeAPI::noise(float frequency, int octaves, float lacunarity, float persistence) {
        return { EC_Volume::noise(frequency, octaves, lacunarity, persistence) };
    }

    VolumeHandle VolumeAPI::sphere(const glm::vec3& center, float radius) { return { EC_Volume::sphere(center, radius) }; }
    VolumeHandle VolumeAPI::box(const glm::vec3& center, const glm::vec3& halfExtents) { return { EC_Volume::box(center, halfExtents) }; }
    VolumeHandle VolumeAPI::halfspace(const glm::vec3& point, const glm::vec3& normal) { return { EC_Volume::halfspace(point, normal) }; }
    VolumeHandle VolumeAPI::cylinder(const glm::vec3& pointA, const glm::vec3& pointB, float radius) { return { EC_Volume::cylinder(pointA, pointB, radius) }; }

    VolumeHandle VolumeAPI::add(VolumeHandle a, VolumeHandle b) { return { EC_Volume::add(a.node, b.node) }; }
    VolumeHandle VolumeAPI::scale(VolumeHandle a, float factor) { return { EC_Volume::scale(a.node, factor) }; }

    VolumeHandle VolumeAPI::unionOf(VolumeHandle a, VolumeHandle b) { return { EC_Volume::unionOf(a.node, b.node) }; }
    VolumeHandle VolumeAPI::intersect(VolumeHandle a, VolumeHandle b) { return { EC_Volume::intersect(a.node, b.node) }; }
    VolumeHandle VolumeAPI::subtract(VolumeHandle a, VolumeHandle b) { return { EC_Volume::subtract(a.node, b.node) }; }

    VolumeHandle VolumeAPI::smoothUnion(VolumeHandle a, VolumeHandle b, float k) { return { EC_Volume::smoothUnion(a.node, b.node, k) }; }
    VolumeHandle VolumeAPI::smoothIntersect(VolumeHandle a, VolumeHandle b, float k) { return { EC_Volume::smoothIntersect(a.node, b.node, k) }; }
    VolumeHandle VolumeAPI::smoothSubtract(VolumeHandle a, VolumeHandle b, float k) { return { EC_Volume::smoothSubtract(a.node, b.node, k) }; }

    VolumeHandle VolumeAPI::translate(VolumeHandle a, const glm::vec3& offset) { return { EC_Volume::translate(a.node, offset) }; }

    void VolumeAPI::setRoot(VolumeHandle root) { m_Root = root.node; }

    void VolumeAPI::setChunkMaterial(const std::string& vertShader, const std::string& fragShader) {
        m_Config.chunkVertShader = vertShader;
        m_Config.chunkFragShader = fragShader;
    }

    void VolumeAPI::setChunkColour(float r, float g, float b, float a) {
        m_Config.chunkColour = glm::vec4(r, g, b, a);
    }

    void VolumeAPI::setGridRadius(int radius) {
        m_Config.gridRadius = radius;
    }
}
