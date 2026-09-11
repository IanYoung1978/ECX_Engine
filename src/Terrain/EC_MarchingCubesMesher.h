#pragma once
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include "Terrain/EC_DensityField.h"
#include "Terrain/EC_TerrainMeshData.h"

// Density-field -> triangle mesh conversion via marching cubes. Pure C++/glm, no engine
// dependency - see the "Marching-cubes voxel terrain mesher" plan for the full design.
namespace EC_MarchingCubesMesher {

// Optional continuous density function, in the SAME local coordinate space as the field
// passed to polygonise (a caller with a chunk-origin offset, e.g. EC_VoxelChunkWorker, is
// responsible for folding that offset into the closure). When supplied, normals are
// computed as a small-epsilon central difference of THIS function directly, instead of a
// central difference of the field's own (coarser, cell-quantized) trilinear interpolation
// - see this header's `polygonise` comment for why that distinction matters.
using DensityFunction = std::function<float(const glm::vec3&)>;

// Walks the field's 32^3 interior cells and emits a triangle soup (no vertex welding -
// each triangle gets 3 fresh vertices). Normals come from a central-difference gradient,
// which is why the field's 1-sample halo matters even for cells right at the interior
// boundary.
//
// analyticDensity (optional): the field itself is already a discretization of some
// underlying continuous shape (typically an EC_VolumeNode tree, baked once per chunk into
// a grid of samples) - fine for placing the isosurface, but differentiating the field's
// own piecewise-trilinear reconstruction of that shape adds cell-boundary noise that isn't
// in the true shape at all. That noise is harmless where the surface is nearly flat, but
// right at a sharp ridge/crest - where the true analytic gradient is still well-defined and
// smoothly-varying, just changing quickly - it was enough to occasionally rotate a
// vertex's computed normal far enough to clip against the light, rendering as a scatter of
// near-black triangles running along ridgelines. Differentiating the original continuous
// function directly (when the caller can provide it) sidesteps that discretization noise
// entirely, since the true shape has no such cell-boundary artifacts. Omit (default
// nullptr) to fall back to the original field-based gradient - e.g. for a caller with only
// a baked field and no live volume tree to hand back.
EC_TerrainMeshData polygonise(const EC_DensityField& field, float isoLevel,
    const DensityFunction& analyticDensity = nullptr);

// Produces `lodCount` variants: LOD 0 is polygonise(field, isoLevel) unchanged; each
// subsequent LOD box-downsamples the field by a factor of 2 (halo included) before
// polygonising, then drops small connected components (a triangle-count-based "clutter"
// filter) from that LOD's output. Coarser LODs are listed later in the returned vector.
// analyticDensity, if given, is reused unchanged for every LOD's normals (it describes the
// same true shape regardless of how coarsely the field approximates it at a given LOD).
std::vector<EC_TerrainMeshData> generateLODs(const EC_DensityField& field, float isoLevel, int lodCount = 3,
    const DensityFunction& analyticDensity = nullptr);

} // namespace EC_MarchingCubesMesher
