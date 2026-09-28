#pragma once
#include <vector>

// Padded 3D scalar field for one voxel chunk: an interiorSize^3 grid plus a 1-sample halo from
// each neighbouring chunk, so gradients (normals) at chunk boundaries are correct and neighbours
// stitch without a seam. Sized dynamically so generateLODs can box-downsample it (32^3 -> 16^3 ->
// 8^3). Low/negative values are solid, matching the marching-cubes convention (val < isoLevel).
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
