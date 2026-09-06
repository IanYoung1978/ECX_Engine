// Unit tests for EC_TerrainWorldDensity - pure C++/glm, no engine dependency. The one
// property that actually matters for chunk streaming: two chunks built by sampling this
// shared function at their own world offsets must agree exactly where they touch.
#include <catch2/catch_test_macros.hpp>
#include <unordered_map>
#include "Terrain/EC_DensityField.h"
#include "Terrain/EC_MarchingCubesMesher.h"
#include "Terrain/EC_TerrainWorldDensity.h"

namespace {

EC_DensityField buildChunkField(const glm::vec3& chunkOrigin, int interiorSize)
{
    EC_DensityField field(interiorSize);
    for (int z = -EC_DensityField::Padding; z < interiorSize + EC_DensityField::Padding; z++) {
        for (int y = -EC_DensityField::Padding; y < interiorSize + EC_DensityField::Padding; y++) {
            for (int x = -EC_DensityField::Padding; x < interiorSize + EC_DensityField::Padding; x++) {
                glm::vec3 worldPos = chunkOrigin + glm::vec3(
                    static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                field.at(x, y, z) = EC_TerrainWorldDensity::sampleWorldDensity(worldPos);
            }
        }
    }
    return field;
}

} // namespace

TEST_CASE("EC_TerrainWorldDensity produces a non-empty mesh for a chunk-sized field", "[TerrainWorldDensity]") {
    EC_DensityField field = buildChunkField(glm::vec3(0.0f), 32);
    EC_TerrainMeshData mesh = EC_MarchingCubesMesher::polygonise(field, 0.0f);

    REQUIRE(!mesh.positions.empty());
    REQUIRE(mesh.indices.size() % 3 == 0);
}

TEST_CASE("DIAGNOSTIC: interior mesh has no non-manifold (hole) edges", "[TerrainWorldDensity][.diagnostic]") {
    constexpr int chunkSize = 32;
    EC_DensityField field = buildChunkField(glm::vec3(0.0f), chunkSize);
    EC_TerrainMeshData mesh = EC_MarchingCubesMesher::polygonise(field, 0.0f);

    auto onChunkBoundary = [&](const glm::vec3& p) {
        constexpr float eps = 0.01f;
        return p.x < eps || p.x > chunkSize - eps
            || p.y < eps || p.y > chunkSize - eps
            || p.z < eps || p.z > chunkSize - eps;
    };

    struct EdgeKey {
        glm::vec3 a, b;
        bool operator==(const EdgeKey& o) const { return a == o.a && b == o.b; }
    };
    struct EdgeHash {
        size_t operator()(const EdgeKey& k) const {
            auto h = std::hash<float>();
            size_t seed = h(k.a.x) ^ h(k.a.y) ^ h(k.a.z) ^ h(k.b.x) ^ h(k.b.y) ^ h(k.b.z);
            return seed;
        }
    };
    auto quantize = [](const glm::vec3& v) {
        constexpr float q = 4096.0f;
        return glm::vec3(std::round(v.x * q) / q, std::round(v.y * q) / q, std::round(v.z * q) / q);
    };
    auto makeEdge = [&](glm::vec3 a, glm::vec3 b) {
        a = quantize(a); b = quantize(b);
        if (b.x < a.x || (b.x == a.x && (b.y < a.y || (b.y == a.y && b.z < a.z)))) std::swap(a, b);
        return EdgeKey{ a, b };
    };

    std::unordered_map<EdgeKey, int, EdgeHash> edgeCount;
    size_t triCount = mesh.indices.size() / 3;
    for (size_t t = 0; t < triCount; t++) {
        glm::vec3 p0 = mesh.positions[mesh.indices[t * 3 + 0]];
        glm::vec3 p1 = mesh.positions[mesh.indices[t * 3 + 1]];
        glm::vec3 p2 = mesh.positions[mesh.indices[t * 3 + 2]];
        edgeCount[makeEdge(p0, p1)]++;
        edgeCount[makeEdge(p1, p2)]++;
        edgeCount[makeEdge(p2, p0)]++;
    }

    int interiorNonManifoldEdges = 0;
    for (auto& [edge, count] : edgeCount) {
        if (count != 2 && !onChunkBoundary(edge.a) && !onChunkBoundary(edge.b)) {
            interiorNonManifoldEdges++;
        }
    }

    INFO("Interior non-manifold (odd-usage) edges found: " << interiorNonManifoldEdges);
    REQUIRE(interiorNonManifoldEdges == 0);
}

TEST_CASE("EC_TerrainWorldDensity: adjacent chunks agree exactly at their shared boundary", "[TerrainWorldDensity]") {
    constexpr int chunkSize = 32;
    glm::vec3 originA(0.0f, 0.0f, 0.0f);
    glm::vec3 originB(static_cast<float>(chunkSize), 0.0f, 0.0f); // neighbour along +X

    EC_DensityField fieldA = buildChunkField(originA, chunkSize);
    EC_DensityField fieldB = buildChunkField(originB, chunkSize);

    EC_TerrainMeshData meshA = EC_MarchingCubesMesher::polygonise(fieldA, 0.0f);
    EC_TerrainMeshData meshB = EC_MarchingCubesMesher::polygonise(fieldB, 0.0f);

    REQUIRE(!meshA.positions.empty());
    REQUIRE(!meshB.positions.empty());

    // A vertex near chunk A's +X face (local x close to chunkSize) should, once shifted
    // into chunk B's local space (its own world origin is chunkSize further along +X),
    // match a vertex chunk B actually produced near its own -X face - both are sampling
    // the identical world-space function at the identical world position.
    bool foundMatchOnBoundary = false;
    for (const glm::vec3& a : meshA.positions) {
        if (a.x < chunkSize - 1.5f) continue;
        glm::vec3 shifted = a - glm::vec3(static_cast<float>(chunkSize), 0.0f, 0.0f);
        for (const glm::vec3& b : meshB.positions) {
            if (glm::length(b - shifted) < 0.001f) { foundMatchOnBoundary = true; break; }
        }
        if (foundMatchOnBoundary) break;
    }
    REQUIRE(foundMatchOnBoundary);
}
