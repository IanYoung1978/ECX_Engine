#pragma once
#include <vector>
#include "Terrain/EC_DensityField.h"
#include "Terrain/EC_TerrainMeshData.h"

// Density-field -> triangle mesh conversion via marching cubes. Pure C++/glm, no engine
// dependency - see the "Marching-cubes voxel terrain mesher" plan for the full design.
namespace EC_MarchingCubesMesher {

// Walks the field's 32^3 interior cells and emits a triangle soup (no vertex welding -
// each triangle gets 3 fresh vertices). Normals come from a central-difference gradient
// of the density field at each vertex position, which is why the field's 1-sample halo
// matters even for cells right at the interior boundary.
EC_TerrainMeshData polygonise(const EC_DensityField& field, float isoLevel);

// Produces `lodCount` variants: LOD 0 is polygonise(field, isoLevel) unchanged; each
// subsequent LOD box-downsamples the field by a factor of 2 (halo included) before
// polygonising, then drops small connected components (a triangle-count-based "clutter"
// filter) from that LOD's output. Coarser LODs are listed later in the returned vector.
std::vector<EC_TerrainMeshData> generateLODs(const EC_DensityField& field, float isoLevel, int lodCount = 3);

} // namespace EC_MarchingCubesMesher
