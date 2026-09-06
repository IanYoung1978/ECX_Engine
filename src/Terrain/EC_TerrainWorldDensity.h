#pragma once
#include <glm/glm.hpp>

// World-space density function shared by every chunk - pure C++/glm, no engine
// dependency, same testability bar as EC_MarchingCubesMesher itself. Any chunk builds its
// own EC_DensityField (interior + 1-voxel halo) by sampling this ONE function at
// `chunkOrigin + localOffset`; since two adjacent chunks sample the identical function at
// matching world positions in their overlap/halo region, their meshes agree exactly at
// the shared boundary with no cross-chunk data exchange needed. Negative = solid,
// positive = open, matching EC_DensityField's documented convention.
namespace EC_TerrainWorldDensity {

float sampleWorldDensity(const glm::vec3& worldPos);

} // namespace EC_TerrainWorldDensity
