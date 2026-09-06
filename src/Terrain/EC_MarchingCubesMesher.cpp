#include "Terrain/EC_MarchingCubesMesher.h"
#include "Terrain/EC_MarchingCubesTables.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>

namespace EC_MarchingCubesMesher {

namespace {

glm::vec3 vertexInterp(float isoLevel, const glm::vec3& p1, const glm::vec3& p2, float v1, float v2)
{
    constexpr float kEps = 1e-6f;
    if (std::abs(v2 - v1) < kEps) return p1;
    float t = (isoLevel - v1) / (v2 - v1);
    t = std::clamp(t, 0.0f, 1.0f);
    return p1 + t * (p2 - p1);
}

// Trilinear sample so the gradient below can probe at fractional (sub-cell) positions.
float sampleTrilinear(const EC_DensityField& field, const glm::vec3& p)
{
    int lo = -EC_DensityField::Padding;
    int hi = field.interiorSize + EC_DensityField::Padding - 1;

    float cx = std::clamp(p.x, static_cast<float>(lo), static_cast<float>(hi));
    float cy = std::clamp(p.y, static_cast<float>(lo), static_cast<float>(hi));
    float cz = std::clamp(p.z, static_cast<float>(lo), static_cast<float>(hi));

    int x0 = static_cast<int>(std::floor(cx));
    int y0 = static_cast<int>(std::floor(cy));
    int z0 = static_cast<int>(std::floor(cz));
    int x1 = std::min(x0 + 1, hi);
    int y1 = std::min(y0 + 1, hi);
    int z1 = std::min(z0 + 1, hi);

    float fx = cx - x0, fy = cy - y0, fz = cz - z0;

    float c000 = field.sample(x0, y0, z0), c100 = field.sample(x1, y0, z0);
    float c010 = field.sample(x0, y1, z0), c110 = field.sample(x1, y1, z0);
    float c001 = field.sample(x0, y0, z1), c101 = field.sample(x1, y0, z1);
    float c011 = field.sample(x0, y1, z1), c111 = field.sample(x1, y1, z1);

    float c00 = c000 * (1 - fx) + c100 * fx;
    float c10 = c010 * (1 - fx) + c110 * fx;
    float c01 = c001 * (1 - fx) + c101 * fx;
    float c11 = c011 * (1 - fx) + c111 * fx;

    float c0 = c00 * (1 - fy) + c10 * fy;
    float c1 = c01 * (1 - fy) + c11 * fy;

    return c0 * (1 - fz) + c1 * fz;
}

// Central-difference gradient of the density field at a (possibly fractional) position.
// The field is low(negative)-inside/high(positive)-outside, so the gradient (direction of
// increasing density) already points from inside to outside - the correct outward surface
// normal - with no sign flip needed.
glm::vec3 computeNormal(const EC_DensityField& field, const glm::vec3& p)
{
    constexpr float e = 0.5f;
    float dx = sampleTrilinear(field, p + glm::vec3(e, 0, 0)) - sampleTrilinear(field, p - glm::vec3(e, 0, 0));
    float dy = sampleTrilinear(field, p + glm::vec3(0, e, 0)) - sampleTrilinear(field, p - glm::vec3(0, e, 0));
    float dz = sampleTrilinear(field, p + glm::vec3(0, 0, e)) - sampleTrilinear(field, p - glm::vec3(0, 0, e));

    glm::vec3 g(dx, dy, dz);
    float len = glm::length(g);
    if (len < 1e-8f) return glm::vec3(0.0f, 1.0f, 0.0f);
    return g / len;
}

// Box-downsample a field to half resolution (interior size halved, halo preserved) by
// averaging 2x2x2 blocks - the simplest correct way to get genuinely coarser geometry for
// a LOD level, rather than a full mesh-space decimation algorithm.
EC_DensityField downsample(const EC_DensityField& field)
{
    int coarseInterior = std::max(2, field.interiorSize / 2);
    EC_DensityField coarse(coarseInterior);

    int lo = -EC_DensityField::Padding;
    int hiFine = field.interiorSize + EC_DensityField::Padding - 1;

    for (int z = -EC_DensityField::Padding; z < coarseInterior + EC_DensityField::Padding; z++) {
        for (int y = -EC_DensityField::Padding; y < coarseInterior + EC_DensityField::Padding; y++) {
            for (int x = -EC_DensityField::Padding; x < coarseInterior + EC_DensityField::Padding; x++) {
                int fx = std::clamp(x * 2, lo, hiFine);
                int fy = std::clamp(y * 2, lo, hiFine);
                int fz = std::clamp(z * 2, lo, hiFine);
                int fx1 = std::clamp(fx + 1, lo, hiFine);
                int fy1 = std::clamp(fy + 1, lo, hiFine);
                int fz1 = std::clamp(fz + 1, lo, hiFine);

                float sum = field.sample(fx, fy, fz) + field.sample(fx1, fy, fz)
                    + field.sample(fx, fy1, fz) + field.sample(fx1, fy1, fz)
                    + field.sample(fx, fy, fz1) + field.sample(fx1, fy, fz1)
                    + field.sample(fx, fy1, fz1) + field.sample(fx1, fy1, fz1);

                coarse.at(x, y, z) = sum / 8.0f;
            }
        }
    }
    return coarse;
}

// Drops small connected components (triangles linked via shared edges) below
// `minTriangles` - removes small isolated blobs/spikes a downsampled field can introduce,
// the "strip clutter/secondary geometry from lower LOD variants" requirement. Rebuilds the
// mesh without welding (matches polygonise()'s own no-welding output), just with the
// dropped triangles' vertices/indices omitted.
EC_TerrainMeshData stripSmallComponents(const EC_TerrainMeshData& mesh, size_t minTriangles)
{
    size_t triCount = mesh.indices.size() / 3;
    if (triCount == 0) return mesh;

    // Union-find over triangles, linked when they share a vertex position (positions
    // aren't welded/shared by index, so adjacency is found by exact position match).
    std::vector<int> parent(triCount);
    for (size_t i = 0; i < triCount; i++) parent[i] = static_cast<int>(i);
    std::function<int(int)> find = [&](int a) {
        while (parent[a] != a) { parent[a] = parent[parent[a]]; a = parent[a]; }
        return a;
    };
    auto unite = [&](int a, int b) {
        a = find(a); b = find(b);
        if (a != b) parent[a] = b;
    };

    struct VecHash {
        size_t operator()(const glm::vec3& v) const {
            auto h = std::hash<float>();
            size_t seed = h(v.x);
            seed ^= h(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };
    std::unordered_map<glm::vec3, int, VecHash> firstTriAtVertex;

    for (size_t t = 0; t < triCount; t++) {
        for (int corner = 0; corner < 3; corner++) {
            const glm::vec3& v = mesh.positions[mesh.indices[t * 3 + corner]];
            auto it = firstTriAtVertex.find(v);
            if (it == firstTriAtVertex.end())
                firstTriAtVertex[v] = static_cast<int>(t);
            else
                unite(static_cast<int>(t), it->second);
        }
    }

    std::unordered_map<int, size_t> componentSize;
    for (size_t t = 0; t < triCount; t++) componentSize[find(static_cast<int>(t))]++;

    EC_TerrainMeshData result;
    for (size_t t = 0; t < triCount; t++) {
        if (componentSize[find(static_cast<int>(t))] < minTriangles) continue;
        for (int corner = 0; corner < 3; corner++) {
            uint32_t srcIndex = mesh.indices[t * 3 + corner];
            result.positions.push_back(mesh.positions[srcIndex]);
            result.normals.push_back(mesh.normals[srcIndex]);
            result.indices.push_back(static_cast<uint32_t>(result.positions.size() - 1));
        }
    }
    return result;
}

// Merges vertices sharing (near-)identical positions into one, rewriting indices to
// reference the shared copy. polygonise() emits a triangle soup (3 fresh vertices per
// triangle, no sharing at all) - fine for the mesher's own correctness (proven by the unit
// tests), but a real problem for anything downstream that assumes ordinary indexed-mesh
// density: e.g. Assimp's aiProcess_JoinIdenticalVertices collapses this soup's massive
// duplicate-position count at every shared cell edge down to a much smaller unique-vertex
// set while leaving the index count unchanged, and any upload code that sizes a vertex
// buffer off the index count (as this engine's ObjModel::initialise() does) reads far past
// the end of the now-much-smaller vertex array. computeNormal() is a pure function of
// position, so two vertices at the same position always get the same normal - keying the
// weld purely on (quantized) position is sufficient, no need to also compare normals.
void weldVertices(EC_TerrainMeshData& mesh)
{
    struct PosHash {
        size_t operator()(const glm::vec3& v) const {
            auto h = std::hash<float>();
            size_t seed = h(v.x);
            seed ^= h(v.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(v.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    constexpr float kQuantize = 4096.0f;
    auto quantize = [kQuantize](const glm::vec3& v) {
        return glm::vec3(std::round(v.x * kQuantize) / kQuantize,
            std::round(v.y * kQuantize) / kQuantize,
            std::round(v.z * kQuantize) / kQuantize);
    };

    std::unordered_map<glm::vec3, uint32_t, PosHash> firstIndexAtPosition;
    EC_TerrainMeshData welded;
    welded.positions.reserve(mesh.positions.size());
    welded.normals.reserve(mesh.normals.size());
    welded.indices.reserve(mesh.indices.size());

    for (uint32_t oldIndex : mesh.indices) {
        glm::vec3 key = quantize(mesh.positions[oldIndex]);
        auto it = firstIndexAtPosition.find(key);
        if (it != firstIndexAtPosition.end()) {
            welded.indices.push_back(it->second);
            continue;
        }
        uint32_t newIndex = static_cast<uint32_t>(welded.positions.size());
        welded.positions.push_back(mesh.positions[oldIndex]);
        welded.normals.push_back(mesh.normals[oldIndex]);
        welded.indices.push_back(newIndex);
        firstIndexAtPosition[key] = newIndex;
    }

    mesh = std::move(welded);
}

// Splits each cube into 6 tetrahedra sharing the cube's main space diagonal (corner 0 to
// corner 6), the standard decomposition that keeps every cube in a uniform grid sliced the
// same way - so two cubes sharing a face always agree on that face's diagonal and the
// surface stitches with no possibility of a crack, unlike the flat 256-case cube table
// this replaced. That table is topologically ambiguous for certain corner configurations
// (some cube cases admit two different, mutually inconsistent triangulations of a shared
// face; the table always picks one, and an adjacent cube can legally pick the other),
// which is what left the scattered interior holes the diagnostic test caught - see
// tests/EC_TerrainWorldDensity_Tests.cpp's "no non-manifold (hole) edges" case. A
// tetrahedron's isosurface has no such ambiguity: a linear field over 4 points crosses an
// isovalue in only one possible way per corner-sign pattern (derived below, not
// table-driven, so there's no transcription-error surface for a bug to hide in).
constexpr int kTetraCorners[6][4] = {
    {0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6},
    {0, 7, 4, 6}, {0, 4, 5, 6}, {0, 5, 1, 6},
};

// Fixes winding the same way the old cube-table path did: pick whichever ordering of the
// triangle's own vertex-normal average, since the gradient-based normals are already known
// to point outward. This matters more here, not less - the tetrahedron cases below are
// derived from data flow, not by hand-checking each triangle's handedness.
void emitTriangle(const EC_DensityField& field, EC_TerrainMeshData& mesh,
    glm::vec3 p0, glm::vec3 p1, glm::vec3 p2)
{
    glm::vec3 n0 = computeNormal(field, p0);
    glm::vec3 n1 = computeNormal(field, p1);
    glm::vec3 n2 = computeNormal(field, p2);

    glm::vec3 windingNormal = glm::cross(p1 - p0, p2 - p0);
    glm::vec3 avgNormal = n0 + n1 + n2;
    if (glm::dot(windingNormal, avgNormal) < 0.0f) {
        std::swap(p1, p2);
        std::swap(n1, n2);
    }

    mesh.positions.push_back(p0); mesh.normals.push_back(n0);
    mesh.positions.push_back(p1); mesh.normals.push_back(n1);
    mesh.positions.push_back(p2); mesh.normals.push_back(n2);
    uint32_t base = static_cast<uint32_t>(mesh.positions.size()) - 3;
    mesh.indices.push_back(base); mesh.indices.push_back(base + 1); mesh.indices.push_back(base + 2);
}

// Polygonises one tetrahedron given its 4 corner positions/values (already resolved from
// the parent cube's 8 corners via kTetraCorners). Every one of the 16 corner-inside
// patterns is one of exactly 3 shapes - empty, a single vertex cut off (1 or 3 corners
// inside), or a planar quad cut (2 and 2) - so the whole case table is this direct
// classification rather than a hardcoded 16-row lookup.
void polygoniseTetrahedron(const EC_DensityField& field, EC_TerrainMeshData& mesh, float isoLevel,
    const glm::vec3 p[4], const float v[4])
{
    int inside = 0;
    for (int c = 0; c < 4; c++)
        if (v[c] < isoLevel) inside |= (1 << c);

    int count = (inside & 1) + ((inside >> 1) & 1) + ((inside >> 2) & 1) + ((inside >> 3) & 1);
    if (count == 0 || count == 4) return;

    if (count == 1 || count == 3) {
        // The one corner whose inside/outside state differs from the other three.
        int apex = -1;
        for (int c = 0; c < 4; c++) {
            bool cIsInside = (inside & (1 << c)) != 0;
            if ((count == 1) == cIsInside) { apex = c; break; }
        }
        glm::vec3 cut[3];
        int slot = 0;
        for (int c = 0; c < 4; c++) {
            if (c == apex) continue;
            cut[slot++] = vertexInterp(isoLevel, p[apex], p[c], v[apex], v[c]);
        }
        emitTriangle(field, mesh, cut[0], cut[1], cut[2]);
        return;
    }

    // count == 2: two corners inside, two outside - the cross-section is a quadrilateral
    // with one corner cut from each of the 4 (inside, outside) pairs. Going P(i0,o0),
    // P(i0,o1), P(i1,o1), P(i1,o0) walks the quad's actual edge loop (each consecutive pair
    // shares a tetrahedron vertex - i0, then o1, then i1, then o0), so splitting it along
    // its own diagonal (i0,o0)-(i1,o1) gives two triangles rather than a bowtie.
    int i0 = -1, i1 = -1, o0 = -1, o1 = -1;
    for (int c = 0; c < 4; c++) {
        bool cIsInside = (inside & (1 << c)) != 0;
        if (cIsInside) { if (i0 < 0) i0 = c; else i1 = c; }
        else { if (o0 < 0) o0 = c; else o1 = c; }
    }
    glm::vec3 q00 = vertexInterp(isoLevel, p[i0], p[o0], v[i0], v[o0]);
    glm::vec3 q01 = vertexInterp(isoLevel, p[i0], p[o1], v[i0], v[o1]);
    glm::vec3 q11 = vertexInterp(isoLevel, p[i1], p[o1], v[i1], v[o1]);
    glm::vec3 q10 = vertexInterp(isoLevel, p[i1], p[o0], v[i1], v[o0]);
    emitTriangle(field, mesh, q00, q01, q11);
    emitTriangle(field, mesh, q00, q11, q10);
}

} // namespace

EC_TerrainMeshData polygonise(const EC_DensityField& field, float isoLevel)
{
    EC_TerrainMeshData mesh;

    for (int z = 0; z < field.interiorSize; z++) {
        for (int y = 0; y < field.interiorSize; y++) {
            for (int x = 0; x < field.interiorSize; x++) {
                glm::vec3 cornerPos[8];
                float cornerVal[8];
                for (int c = 0; c < 8; c++) {
                    int cx = x + static_cast<int>(EC_MarchingCubesTables::kCubeCornerOffset[c][0]);
                    int cy = y + static_cast<int>(EC_MarchingCubesTables::kCubeCornerOffset[c][1]);
                    int cz = z + static_cast<int>(EC_MarchingCubesTables::kCubeCornerOffset[c][2]);
                    cornerPos[c] = glm::vec3(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(cz));
                    cornerVal[c] = field.sample(cx, cy, cz);
                }

                for (const auto& tet : kTetraCorners) {
                    glm::vec3 tetPos[4] = { cornerPos[tet[0]], cornerPos[tet[1]], cornerPos[tet[2]], cornerPos[tet[3]] };
                    float tetVal[4] = { cornerVal[tet[0]], cornerVal[tet[1]], cornerVal[tet[2]], cornerVal[tet[3]] };
                    polygoniseTetrahedron(field, mesh, isoLevel, tetPos, tetVal);
                }
            }
        }
    }

    weldVertices(mesh);
    return mesh;
}

std::vector<EC_TerrainMeshData> generateLODs(const EC_DensityField& field, float isoLevel, int lodCount)
{
    std::vector<EC_TerrainMeshData> lods;
    lods.reserve(std::max(lodCount, 1));

    lods.push_back(polygonise(field, isoLevel));

    EC_DensityField current = field;
    for (int lod = 1; lod < lodCount; lod++) {
        current = downsample(current);
        EC_TerrainMeshData mesh = polygonise(current, isoLevel);
        // Threshold scales down with the coarser grid's naturally lower triangle budget.
        size_t minTriangles = std::max<size_t>(2, static_cast<size_t>(current.interiorSize));
        lods.push_back(stripSmallComponents(mesh, minTriangles));
        if (current.interiorSize <= 2) break;
    }

    return lods;
}

} // namespace EC_MarchingCubesMesher
