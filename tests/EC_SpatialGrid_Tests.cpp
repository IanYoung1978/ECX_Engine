// Unit tests for EC_SpatialGrid - a self-contained uniform-cell spatial index
// keyed only on entity id + world AABB. No engine dependency beyond glm and
// EC_DOD_Types (just a uint32_t alias).
#include <catch2/catch_test_macros.hpp>
#include "Spatial/EC_SpatialGrid.h"

TEST_CASE("EC_SpatialGrid queryAABB finds entities inserted into overlapping cells", "[SpatialGrid]") {
    EC_SpatialGrid grid(10.0f);

    grid.insert(1, AABB{ glm::vec3(-1.0f), glm::vec3(1.0f) });   // cell (0,0,0) region
    grid.insert(2, AABB{ glm::vec3(50.0f), glm::vec3(52.0f) });  // far away

    auto hits = grid.queryAABB(AABB{ glm::vec3(-2.0f), glm::vec3(2.0f) });

    REQUIRE(hits.size() == 1);
    REQUIRE(hits[0] == 1);
}

TEST_CASE("EC_SpatialGrid query region with nothing nearby returns empty", "[SpatialGrid]") {
    EC_SpatialGrid grid(10.0f);
    grid.insert(1, AABB{ glm::vec3(-1.0f), glm::vec3(1.0f) });

    auto hits = grid.queryAABB(AABB{ glm::vec3(500.0f), glm::vec3(502.0f) });

    REQUIRE(hits.empty());
}

TEST_CASE("EC_SpatialGrid an entity spanning multiple cells is found from either cell", "[SpatialGrid]") {
    EC_SpatialGrid grid(10.0f);
    // Straddles the boundary between cell (0,0,0) and (1,0,0).
    grid.insert(7, AABB{ glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(15.0f, 1.0f, 1.0f) });

    auto hitsLeftCell = grid.queryAABB(AABB{ glm::vec3(0.0f), glm::vec3(1.0f) });
    auto hitsRightCell = grid.queryAABB(AABB{ glm::vec3(12.0f, 0.0f, 0.0f), glm::vec3(13.0f, 1.0f, 1.0f) });

    REQUIRE(hitsLeftCell.size() == 1);
    REQUIRE(hitsRightCell.size() == 1);
    REQUIRE(hitsLeftCell[0] == 7);
    REQUIRE(hitsRightCell[0] == 7);
}

TEST_CASE("EC_SpatialGrid clear() empties all previously inserted entities", "[SpatialGrid]") {
    EC_SpatialGrid grid(10.0f);
    grid.insert(1, AABB{ glm::vec3(-1.0f), glm::vec3(1.0f) });

    grid.clear();
    auto hits = grid.queryAABB(AABB{ glm::vec3(-1.0f), glm::vec3(1.0f) });

    REQUIRE(hits.empty());
}

TEST_CASE("EC_SpatialGrid duplicate inserts of the same entity across overlapping cells still dedupe on query", "[SpatialGrid]") {
    EC_SpatialGrid grid(10.0f);
    // AABB spans 4 cells on X/Z at cell size 10; query region overlapping all of them
    // should report entity 3 exactly once (queryAABB dedupes via unordered_set).
    grid.insert(3, AABB{ glm::vec3(-5.0f, 0.0f, -5.0f), glm::vec3(5.0f, 1.0f, 5.0f) });

    auto hits = grid.queryAABB(AABB{ glm::vec3(-15.0f), glm::vec3(15.0f) });

    REQUIRE(hits.size() == 1);
}
