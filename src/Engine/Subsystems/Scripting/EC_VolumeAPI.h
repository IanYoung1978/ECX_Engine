#pragma once
#include "Procedural/EC_VolumeNode.h"
#include <glm/glm.hpp>
#include <string>

namespace ScriptAPI
{
    // Everything EC_VoxelChunkSystem needs besides the shape itself (see VolumeAPI::getRoot)
    // to spawn/mesh/render voxel terrain - previously hardcoded C++ constants
    // (kGridRadius, the chunk shader paths, the tint colour) in EC_VoxelChunkSystem.cpp
    // (issue #99). Defaults here match that old hardcoded behaviour exactly, so a
    // generation script that doesn't call setChunkMaterial()/setChunkColour()/
    // setGridRadius() gets today's look unchanged.
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

    // The `volume` global - an author-facing toolbox of composable shape/noise primitives
    // (Stage 1: "create a volume", Stage 2: "subtract from it" - see the terrain generation
    // plan). One instance, pushed as a Lua global the same way GameAPI is - see
    // EC_LuaScriptSystem::registerAPI(). setRoot()/getRoot() capture the finished tree for
    // the engine to retrieve once the authoring script finishes running.
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

        // Issue #99 - author-facing config a generation script can set alongside setRoot(),
        // instead of the engine hardcoding a chunk material/tint/grid size for every game.
        // All optional; see VoxelTerrainConfig's own comment for the defaults used if a
        // script doesn't call these.
        void setChunkMaterial(const std::string& vertShader, const std::string& fragShader);
        void setChunkColour(float r, float g, float b, float a);
        void setGridRadius(int radius);
        const VoxelTerrainConfig& getConfig() const { return m_Config; }

    private:
        EC_VolumeNodePtr m_Root;
        VoxelTerrainConfig m_Config;
    };
}
