#pragma once

// Marching cubes lookup tables - corner/edge numbering and the 256-case triangle table -
// sourced directly from https://paulbourke.net/geometry/polygonise/ (the reference this
// module is built against). kCubeCornerOffset/kCubeEdgeCorners are the classic numbering
// described on that page (matches Cory Bloyd's marchingsource.cpp); kTriangleTable is
// Geoffrey Heller's alternative encoding from the same page (table2.txt), verified against
// that page's own worked examples (case 8 -> edges {11,2,3}, case 9 -> edges {11,2,0,8})
// to confirm it uses the identical corner/edge convention before use here.
namespace EC_MarchingCubesTables {

// Position of each of a cube's 8 corners, relative to the cube's minimum corner, in units
// of one cell (multiply by cell size before use).
inline constexpr float kCubeCornerOffset[8][3] = {
    {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f},
};

// The two corner indices each of a cube's 12 edges connects.
inline constexpr int kCubeEdgeCorners[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7},
};

// Indexed by an 8-bit cube case (bit i set => corner i is "inside", val < isoLevel).
// Each row lists edge indices (0-11) forming triangles in groups of 3, terminated by -1.
// At most 4 triangles (13 slots: up to 12 edge indices + terminator).
extern const int kTriangleTable[256][13];

} // namespace EC_MarchingCubesTables
