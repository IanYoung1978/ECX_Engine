// Unit tests for EC_Frustum plane extraction + AABB culling. Pure glm, no
// engine/GL dependency (view/projection matrices are built by hand here rather
// than pulled from a live camera).
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Spatial/EC_Frustum.h"

using Catch::Matchers::WithinAbs;

namespace {
    glm::mat4 standardViewProjection() {
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
        return proj * view;
    }
}

TEST_CASE("extractPlanes produces 6 unit-length plane normals", "[Frustum]") {
    auto planes = EC_Frustum::extractPlanes(standardViewProjection());
    for (const auto& p : planes) {
        REQUIRE_THAT(glm::length(p.normal), WithinAbs(1.0f, 1e-3f));
    }
}

TEST_CASE("intersectsAABB keeps a box centered in front of the camera", "[Frustum]") {
    auto planes = EC_Frustum::extractPlanes(standardViewProjection());
    AABB box{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f) };

    REQUIRE(EC_Frustum::intersectsAABB(planes, box));
}

TEST_CASE("intersectsAABB culls a box far behind the camera (outside the near/far range)", "[Frustum]") {
    auto planes = EC_Frustum::extractPlanes(standardViewProjection());
    AABB box{ glm::vec3(-1.0f, -1.0f, 999.0f), glm::vec3(1.0f, 1.0f, 1001.0f) };

    REQUIRE_FALSE(EC_Frustum::intersectsAABB(planes, box));
}

TEST_CASE("intersectsAABB culls a box far outside the left/right side planes", "[Frustum]") {
    auto planes = EC_Frustum::extractPlanes(standardViewProjection());
    AABB box{ glm::vec3(500.0f, -1.0f, -1.0f), glm::vec3(501.0f, 1.0f, 1.0f) };

    REQUIRE_FALSE(EC_Frustum::intersectsAABB(planes, box));
}
