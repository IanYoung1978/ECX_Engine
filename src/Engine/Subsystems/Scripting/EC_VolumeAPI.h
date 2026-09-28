#pragma once
#include "Procedural/EC_VolumeNode.h"
#include <glm/glm.hpp>
#include <string>

namespace ScriptAPI
{
    // Everything EC_VoxelChunkSystem needs besides the shape (see VolumeAPI::getRoot). A generation
    // script that doesn't call setChunkMaterial()/setChunkColour()/setGridRadius() gets these defaults.
    struct VoxelTerrainConfig
    {
        std::string chunkVertShader = "data/assets/shaders/basic.vert";
        std::string chunkFragShader = "data/assets/shaders/PBR.frag";
        glm::vec4 chunkColour{ 0.5f, 0.45f, 0.35f, 1.0f };
        int gridRadius = 1;
    };

    // Lua-visible copyable wrapper around a shared_ptr<EC_VolumeNode> - registered as a
    // LuaBridge value type exactly like glm::vec3/vec4 (see EC_LuaScriptingSystem's
    // registerAPI()), so every volume.* call below can take/return it and Lua composition
    // reads naturally: `volume.union(volume.sphere(...), volume.box(...))`.
    struct VolumeHandle
    {
        EC_VolumeNodePtr node;
    };

    // The `volume` global: composable shape/noise primitives, registered like GameAPI (see
    // EC_LuaScriptSystem::registerAPI()). setRoot()/getRoot() capture the finished tree for the engine
    // once the script ends.
    struct VolumeAPI
    {
        VolumeHandle constant(float value);
        VolumeHandle noise(float frequency, int octaves, float lacunarity, float persistence);
        VolumeHandle sphere(const glm::vec3& center, float radius);
        VolumeHandle box(const glm::vec3& center, const glm::vec3& halfExtents);
        VolumeHandle halfspace(const glm::vec3& point, const glm::vec3& normal);
        VolumeHandle cylinder(const glm::vec3& pointA, const glm::vec3& pointB, float radius);

        VolumeHandle add(VolumeHandle a, VolumeHandle b);
        VolumeHandle scale(VolumeHandle a, float factor);

        VolumeHandle unionOf(VolumeHandle a, VolumeHandle b);
        VolumeHandle intersect(VolumeHandle a, VolumeHandle b);
        VolumeHandle subtract(VolumeHandle a, VolumeHandle b);

        VolumeHandle smoothUnion(VolumeHandle a, VolumeHandle b, float k);
        VolumeHandle smoothIntersect(VolumeHandle a, VolumeHandle b, float k);
        VolumeHandle smoothSubtract(VolumeHandle a, VolumeHandle b, float k);

        VolumeHandle translate(VolumeHandle a, const glm::vec3& offset);

        void setRoot(VolumeHandle root);
        EC_VolumeNodePtr getRoot() const { return m_Root; }

        // Optional config a generation script can set alongside setRoot(); see VoxelTerrainConfig for the
        // defaults.
        void setChunkMaterial(const std::string& vertShader, const std::string& fragShader);
        void setChunkColour(float r, float g, float b, float a);
        void setGridRadius(int radius);
        const VoxelTerrainConfig& getConfig() const { return m_Config; }

    private:
        EC_VolumeNodePtr m_Root;
        VoxelTerrainConfig m_Config;
    };
}
