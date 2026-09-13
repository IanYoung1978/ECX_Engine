// Unit tests for mesh-accurate ray/cone collision (Mesh-accurate terrain collision plan):
// EC_RayIntersection::rayMesh must hit the real triangles of a retained chunk mesh, not
// just its bounding box.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Engine/Subsystems/CollisionSystems/EC_RayIntersection.h"

using Catch::Matchers::WithinAbs;

namespace {
    constexpr float kTol = 1e-4f;

    // A single flat quad (two triangles) spanning x/z in [-1,1] at y = 2, normal +Y -
    // enough to distinguish "hit the real surface" from "hit the bounding box top/bottom".
    EC_DOD_MeshCollisionData makeFlatQuad(float y) {
        EC_DOD_MeshCollisionData mesh;
        mesh.positions = std::make_shared<std::vector<glm::vec3>>(std::vector<glm::vec3>{
            glm::vec3(-1.0f, y, -1.0f),
            glm::vec3(1.0f, y, -1.0f),
            glm::vec3(1.0f, y, 1.0f),
            glm::vec3(-1.0f, y, 1.0f),
        });
        mesh.normals = std::make_shared<std::vector<glm::vec3>>(std::vector<glm::vec3>{
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
        });
        mesh.indices = std::make_shared<std::vector<uint32_t>>(std::vector<uint32_t>{
            0, 1, 2,
            0, 2, 3,
        });
        return mesh;
    }
}

TEST_CASE("rayMesh hits the real triangle surface, not just a bounding-box approximation", "[RayIntersection][Mesh]") {
    EC_DOD_MeshCollisionData mesh = makeFlatQuad(2.0f);

    SECTION("ray straight down through the quad's interior hits at the mesh's own height") {
        RayIntersectionResult result = EC_RayIntersection::rayMesh(
            glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), mesh);
        REQUIRE(result.hit);
        REQUIRE_THAT(result.position.y, WithinAbs(2.0f, kTol));
        REQUIRE_THAT(result.distance, WithinAbs(8.0f, kTol));
        REQUIRE_THAT(result.normal.y, WithinAbs(1.0f, kTol));
    }

    SECTION("ray outside the quad's footprint (but still inside a bounding box around it) misses") {
        // x = 5 is well outside the quad's [-1,1] extent, but within a naive AABB that
        // only bounded the quad loosely on x - proving this is triangle-accurate.
        RayIntersectionResult result = EC_RayIntersection::rayMesh(
            glm::vec3(5.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), mesh);
        REQUIRE_FALSE(result.hit);
    }

    SECTION("ray hitting the second triangle of the quad also resolves correctly") {
        RayIntersectionResult result = EC_RayIntersection::rayMesh(
            glm::vec3(0.5f, 10.0f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f), mesh);
        REQUIRE(result.hit);
        REQUIRE_THAT(result.position.y, WithinAbs(2.0f, kTol));
    }

    SECTION("closest of two overlapping triangles wins") {
        EC_DOD_MeshCollisionData nearMesh = makeFlatQuad(2.0f);
        EC_DOD_MeshCollisionData farMesh = makeFlatQuad(-2.0f);
        // Merge into one mesh: near quad's two triangles, then far quad's two triangles.
        EC_DOD_MeshCollisionData combined;
        combined.positions = std::make_shared<std::vector<glm::vec3>>();
        combined.normals = std::make_shared<std::vector<glm::vec3>>();
        combined.indices = std::make_shared<std::vector<uint32_t>>();
        combined.positions->insert(combined.positions->end(), nearMesh.positions->begin(), nearMesh.positions->end());
        combined.positions->insert(combined.positions->end(), farMesh.positions->begin(), farMesh.positions->end());
        combined.normals->insert(combined.normals->end(), nearMesh.normals->begin(), nearMesh.normals->end());
        combined.normals->insert(combined.normals->end(), farMesh.normals->begin(), farMesh.normals->end());
        for (uint32_t idx : *nearMesh.indices) combined.indices->push_back(idx);
        for (uint32_t idx : *farMesh.indices) combined.indices->push_back(idx + 4);

        RayIntersectionResult result = EC_RayIntersection::rayMesh(
            glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), combined);
        REQUIRE(result.hit);
        REQUIRE_THAT(result.position.y, WithinAbs(2.0f, kTol));
    }

    SECTION("empty mesh data never hits") {
        EC_DOD_MeshCollisionData empty;
        RayIntersectionResult result = EC_RayIntersection::rayMesh(
            glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), empty);
        REQUIRE_FALSE(result.hit);
    }
}

TEST_CASE("rayVsCollider dispatches Mesh colliders to rayMesh", "[RayIntersection][Mesh]") {
    EC_DOD_Collider collider;
    collider.type = EC_DOD_Collider::Type::Mesh;
    EC_DOD_Spatial spatial;
    EC_DOD_MeshCollisionData mesh = makeFlatQuad(3.0f);

    RayIntersectionResult result = EC_RayIntersection::rayVsCollider(
        glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), collider, spatial, mesh);
    REQUIRE(result.hit);
    REQUIRE_THAT(result.position.y, WithinAbs(3.0f, kTol));
}

TEST_CASE("rayVsCollider offsets a Mesh collider's chunk-local vertices by spatial.position", "[RayIntersection][Mesh]") {
    // Regression test: EC_TerrainMeshData/EC_DOD_MeshCollisionData positions are
    // chunk-local (0..kChunkWorldSize), not world space - a chunk at a non-zero world
    // position was silently tested at the wrong location before this offset was added
    // (every chunk except the one sitting exactly at world origin was affected).
    EC_DOD_Collider collider;
    collider.type = EC_DOD_Collider::Type::Mesh;
    EC_DOD_Spatial spatial;
    spatial.position = glm::vec3(100.0f, 0.0f, 200.0f);
    // Quad authored in local space at local y=3 - with the spatial offset applied, its
    // world-space height is 3, same as the origin-chunk case above.
    EC_DOD_MeshCollisionData mesh = makeFlatQuad(3.0f);

    RayIntersectionResult result = EC_RayIntersection::rayVsCollider(
        glm::vec3(100.0f, 10.0f, 200.0f), glm::vec3(0.0f, -1.0f, 0.0f), collider, spatial, mesh);
    REQUIRE(result.hit);
    REQUIRE_THAT(result.position.x, WithinAbs(100.0f, kTol));
    REQUIRE_THAT(result.position.y, WithinAbs(3.0f, kTol));
    REQUIRE_THAT(result.position.z, WithinAbs(200.0f, kTol));

    // A ray at the SAME local coordinates (0,10,0)/(0,-1,0) but NOT offset into this
    // chunk's world position must miss - proving the offset is actually applied, not
    // coincidentally passing because local and world happened to line up.
    RayIntersectionResult missResult = EC_RayIntersection::rayVsCollider(
        glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), collider, spatial, mesh);
    REQUIRE_FALSE(missResult.hit);
}
