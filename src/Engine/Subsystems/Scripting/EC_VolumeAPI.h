#pragma once
#include "Procedural/EC_VolumeNode.h"
#include <glm/glm.hpp>

namespace ScriptAPI
{
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

    private:
        EC_VolumeNodePtr m_Root;
    };
}
