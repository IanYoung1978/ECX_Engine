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

                int cubeIndex = 0;
                for (int c = 0; c < 8; c++)
                    if (cornerVal[c] < isoLevel) cubeIndex |= (1 << c);

                const int* row = EC_MarchingCubesTables::kTriangleTable[cubeIndex];
                if (row[0] < 0) continue;

                for (int i = 0; i < 13 && row[i] >= 0; i += 3) {
                    glm::vec3 triPos[3];
                    glm::vec3 triNormal[3];
                    for (int corner = 0; corner < 3; corner++) {
                        int edge = row[i + corner];
                        int a = EC_MarchingCubesTables::kCubeEdgeCorners[edge][0];
                        int b = EC_MarchingCubesTables::kCubeEdgeCorners[edge][1];
                        triPos[corner] = vertexInterp(isoLevel, cornerPos[a], cornerPos[b], cornerVal[a], cornerVal[b]);
                        triNormal[corner] = computeNormal(field, triPos[corner]);
                    }

                    // The table's case rows don't all share one consistent winding
                    // direction (some cube configurations emit CW, others CCW, relative to
                    // the surface's actual outward side) - left as-is, the renderer's
                    // backface culling would then remove a different, camera-angle-
                    // dependent subset of triangles across the mesh, which looks exactly
                    // like a lighting boundary that rotates with the camera (culling
                    // happens before the fragment shader runs, so no shading fix could
                    // have masked or revealed this). Correct it per-triangle: if the
                    // geometric winding normal disagrees with the vertex normals (already
                    // proven correctly outward-facing), swap two corners to flip it.
                    glm::vec3 windingNormal = glm::cross(triPos[1] - triPos[0], triPos[2] - triPos[0]);
                    glm::vec3 avgNormal = triNormal[0] + triNormal[1] + triNormal[2];
                    if (glm::dot(windingNormal, avgNormal) < 0.0f) {
                        std::swap(triPos[1], triPos[2]);
                        std::swap(triNormal[1], triNormal[2]);
                    }

                    for (int corner = 0; corner < 3; corner++) {
                        mesh.positions.push_back(triPos[corner]);
                        mesh.normals.push_back(triNormal[corner]);
                        mesh.indices.push_back(static_cast<uint32_t>(mesh.positions.size() - 1));
                    }
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
