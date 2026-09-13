#pragma once
#include <vector>

// A padded 3D scalar field for one voxel chunk. The interior is an `interiorSize`^3 grid
// of cells (the default chunk size is 32), and every side additionally carries a 1-sample
// halo copied from the corresponding neighbouring chunk - this is what lets
// EC_MarchingCubesMesher compute correct gradients (and thus normals) for vertices right
// at a chunk's boundary, and lets a future chunk streamer stitch adjacent chunks without a
// seam. See "Marching-cubes voxel terrain mesher" plan's density-field section.
//
// Sized dynamically (not a fixed std::array) so EC_MarchingCubesMesher::generateLODs can
// build progressively smaller fields for coarser LODs by box-downsampling a 32^3 field
// down to e.g. 16^3, 8^3, reusing this same type at every level.
//
// Convention: low/negative values are "inside" the surface (solid) - e.g. a signed-
// distance-style field, `length(p - center) - radius` for a sphere - matching the classic
// marching cubes convention (`if (val < isoLevel) cubeindex |= bit`) used by
// EC_MarchingCubesMesher and its lookup tables.
struct EC_DensityField {
    static constexpr int Padding = 1;

    int interiorSize;
    int size; // interiorSize + 2 * Padding
    std::vector<float> values;

    explicit EC_DensityField(int interiorSize_ = 32)
        : interiorSize(interiorSize_)
        , size(interiorSize_ + 2 * Padding)
        , values(static_cast<size_t>(size) * size * size, 0.0f)
    {
    }

    // x, y, z range over [-Padding, interiorSize + Padding - 1].
    size_t index(int x, int y, int z) const {
        int ix = x + Padding;
        int iy = y + Padding;
        int iz = z + Padding;
        return (static_cast<size_t>(iz) * size + iy) * size + ix;
    }

    float& at(int x, int y, int z) { return values[index(x, y, z)]; }
    float sample(int x, int y, int z) const { return values[index(x, y, z)]; }
};
