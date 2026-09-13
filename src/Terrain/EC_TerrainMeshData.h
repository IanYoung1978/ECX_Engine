#pragma once
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

// Output of EC_MarchingCubesMesher::polygonise() - deliberately its own lightweight
// struct rather than reusing ObjModel's 7-vector/Assimp-oriented layout (no UV/tangent/
// bitangent data makes sense for procedurally generated terrain, at least until
// texture-blending is added later).
struct EC_TerrainMeshData {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
    // Forward-compatible plumbing only (Stage 3 of the terrain generation plan: "assign
    // game values to what's left") - always 0 for now. No authoring API for this yet;
    // real per-vertex material assignment (an ordered region->materialId stack, reusing
    // EC_VolumeNode as the "region") is a separate, not-yet-designed follow-up. This field
    // exists so that design isn't blocked by a missing data-model seam later.
    std::vector<uint32_t> materialIds;
};
