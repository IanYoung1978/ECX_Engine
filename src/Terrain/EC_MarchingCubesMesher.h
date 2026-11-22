#pragma once
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include "Terrain/EC_DensityField.h"
#include "Terrain/EC_TerrainMeshData.h"

// Density-field -> triangle mesh conversion via marching cubes. Pure C++/glm, no engine
// dependency - see the "Marching-cubes voxel terrain mesher" plan for the full design.
namespace EC_MarchingCubesMesher {

// Optional continuous density function in the same local space as the field (fold any chunk-origin
// offset into the closure). When given, normals are a central difference of this function rather than
// of the field's coarser trilinear interpolation (see polygonise).
using DensityFunction = std::function<float(const glm::vec3&)>;

// Walks the field's interior cells and emits a triangle soup (no welding); normals are
// central-difference gradients, hence the halo. analyticDensity (optional) is the original continuous
// function, which avoids cell-boundary noise in the baked gradient that can flip normals along
// ridgelines. Omit it to use the field gradient.
EC_TerrainMeshData polygonise(const EC_DensityField& field, float isoLevel,
    const DensityFunction& analyticDensity = nullptr);

// Produces `lodCount` variants: LOD 0 is polygonise(field, isoLevel); each later LOD
// box-downsamples the field by 2 (halo included), polygonises, then drops small connected components.
// Coarser LODs come later. analyticDensity, if given, is reused for every LOD's normals.
std::vector<EC_TerrainMeshData> generateLODs(const EC_DensityField& field, float isoLevel, int lodCount = 3,
    const DensityFunction& analyticDensity = nullptr);

} // namespace EC_MarchingCubesMesher
