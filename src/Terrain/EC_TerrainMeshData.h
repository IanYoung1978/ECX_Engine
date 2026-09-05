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
};
