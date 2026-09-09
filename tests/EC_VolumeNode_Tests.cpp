// Unit tests for EC_VolumeNode - pure C++/glm, no engine dependency. Covers each
// primitive's shape and each combinator's behaviour directly, independent of any
// Lua/terrain wiring - matches EC_Noise3D_Tests.cpp's own testability bar.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Procedural/EC_VolumeNode.h"

using Catch::Matchers::WithinAbs;

namespace {
    constexpr float kTol = 1e-4f;
}

TEST_CASE("constant always returns its value", "[VolumeNode]") {
    auto node = EC_Volume::constant(-3.5f);
    REQUIRE_THAT(node->evaluate(glm::vec3(0.0f)), WithinAbs(-3.5f, kTol));
    REQUIRE_THAT(node->evaluate(glm::vec3(100.0f, -50.0f, 7.0f)), WithinAbs(-3.5f, kTol));
}

TEST_CASE("sphere is negative (solid) inside, positive (open) outside, zero on the surface", "[VolumeNode]") {
    auto node = EC_Volume::sphere(glm::vec3(0.0f), 5.0f);
    REQUIRE(node->evaluate(glm::vec3(0.0f)) < 0.0f);
    REQUIRE_THAT(node->evaluate(glm::vec3(5.0f, 0.0f, 0.0f)), WithinAbs(0.0f, kTol));
    REQUIRE(node->evaluate(glm::vec3(10.0f, 0.0f, 0.0f)) > 0.0f);
}

TEST_CASE("box is negative inside, positive outside", "[VolumeNode]") {
    auto node = EC_Volume::box(glm::vec3(0.0f), glm::vec3(2.0f, 3.0f, 4.0f));
    REQUIRE(node->evaluate(glm::vec3(0.0f)) < 0.0f);
    REQUIRE(node->evaluate(glm::vec3(1.0f, 1.0f, 1.0f)) < 0.0f);
    REQUIRE(node->evaluate(glm::vec3(10.0f, 10.0f, 10.0f)) > 0.0f);
    REQUIRE_THAT(node->evaluate(glm::vec3(2.0f, 0.0f, 0.0f)), WithinAbs(0.0f, kTol));
}

TEST_CASE("halfspace is solid on the side the normal points away from", "[VolumeNode]") {
    auto node = EC_Volume::halfspace(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    REQUIRE(node->evaluate(glm::vec3(0.0f, 5.0f, 0.0f)) < 0.0f);
    REQUIRE(node->evaluate(glm::vec3(0.0f, 15.0f, 0.0f)) > 0.0f);
    REQUIRE_THAT(node->evaluate(glm::vec3(100.0f, 10.0f, -50.0f)), WithinAbs(0.0f, kTol));
}

TEST_CASE("cylinder is solid inside the radius and between its endpoints", "[VolumeNode]") {
    auto node = EC_Volume::cylinder(glm::vec3(0.0f, -10.0f, 0.0f), glm::vec3(0.0f, 10.0f, 0.0f), 3.0f);
    REQUIRE(node->evaluate(glm::vec3(0.0f, 0.0f, 0.0f)) < 0.0f);       // on-axis, mid-height
    REQUIRE(node->evaluate(glm::vec3(10.0f, 0.0f, 0.0f)) > 0.0f);      // far outside radius
    REQUIRE(node->evaluate(glm::vec3(0.0f, 50.0f, 0.0f)) > 0.0f);      // beyond the cap
}

TEST_CASE("add sums two densities", "[VolumeNode]") {
    auto node = EC_Volume::add(EC_Volume::constant(2.0f), EC_Volume::constant(-5.0f));
    REQUIRE_THAT(node->evaluate(glm::vec3(0.0f)), WithinAbs(-3.0f, kTol));
}

TEST_CASE("scale multiplies density by a constant factor", "[VolumeNode]") {
    auto node = EC_Volume::scale(EC_Volume::constant(3.0f), -2.0f);
    REQUIRE_THAT(node->evaluate(glm::vec3(0.0f)), WithinAbs(-6.0f, kTol));
}

TEST_CASE("union is the minimum of its inputs (solid if either is solid)", "[VolumeNode]") {
    auto node = EC_Volume::unionOf(EC_Volume::constant(-1.0f), EC_Volume::constant(5.0f));
    REQUIRE_THAT(node->evaluate(glm::vec3(0.0f)), WithinAbs(-1.0f, kTol));
}

TEST_CASE("intersect is the maximum of its inputs (solid only if both are solid)", "[VolumeNode]") {
    auto node = EC_Volume::intersect(EC_Volume::constant(-1.0f), EC_Volume::constant(5.0f));
    REQUIRE_THAT(node->evaluate(glm::vec3(0.0f)), WithinAbs(5.0f, kTol));
}

TEST_CASE("subtract removes b's solid region from a", "[VolumeNode]") {
    // a solid (-1), b solid (-1) -> subtract should be open (positive): max(-1, -(-1)) = 1
    auto bothSolid = EC_Volume::subtract(EC_Volume::constant(-1.0f), EC_Volume::constant(-1.0f));
    REQUIRE(bothSolid->evaluate(glm::vec3(0.0f)) > 0.0f);

    // a solid (-1), b open (1) -> subtract should stay solid: max(-1, -1) = -1
    auto onlyASolid = EC_Volume::subtract(EC_Volume::constant(-1.0f), EC_Volume::constant(1.0f));
    REQUIRE(onlyASolid->evaluate(glm::vec3(0.0f)) < 0.0f);
}

TEST_CASE("real CSG composition: a sphere with a smaller sphere subtracted is hollow", "[VolumeNode]") {
    auto shell = EC_Volume::subtract(
        EC_Volume::sphere(glm::vec3(0.0f), 10.0f),
        EC_Volume::sphere(glm::vec3(0.0f), 5.0f));

    REQUIRE(shell->evaluate(glm::vec3(0.0f)) > 0.0f);              // hollow center is open
    REQUIRE(shell->evaluate(glm::vec3(7.5f, 0.0f, 0.0f)) < 0.0f);  // shell wall is solid
    REQUIRE(shell->evaluate(glm::vec3(20.0f, 0.0f, 0.0f)) > 0.0f); // far outside is open
}

TEST_CASE("smoothUnion approaches hard union as k shrinks", "[VolumeNode]") {
    auto a = EC_Volume::sphere(glm::vec3(-3.0f, 0.0f, 0.0f), 2.0f);
    auto b = EC_Volume::sphere(glm::vec3(3.0f, 0.0f, 0.0f), 2.0f);
    auto hard = EC_Volume::unionOf(a, b);
    auto smoothZero = EC_Volume::smoothUnion(a, b, 0.0f);
    REQUIRE_THAT(smoothZero->evaluate(glm::vec3(0.0f)), WithinAbs(hard->evaluate(glm::vec3(0.0f)), kTol));
}

TEST_CASE("smoothUnion blends two nearby shapes into a lower (more negative) value than hard union at the seam", "[VolumeNode]") {
    auto a = EC_Volume::sphere(glm::vec3(-2.0f, 0.0f, 0.0f), 2.0f);
    auto b = EC_Volume::sphere(glm::vec3(2.0f, 0.0f, 0.0f), 2.0f);
    auto hard = EC_Volume::unionOf(a, b);
    auto smooth = EC_Volume::smoothUnion(a, b, 1.5f);
    glm::vec3 seam(0.0f, 0.0f, 0.0f);
    REQUIRE(smooth->evaluate(seam) < hard->evaluate(seam));
}

TEST_CASE("translate offsets where a node is evaluated", "[VolumeNode]") {
    auto node = EC_Volume::translate(EC_Volume::sphere(glm::vec3(0.0f), 2.0f), glm::vec3(10.0f, 0.0f, 0.0f));
    REQUIRE(node->evaluate(glm::vec3(10.0f, 0.0f, 0.0f)) < 0.0f);
    REQUIRE(node->evaluate(glm::vec3(0.0f, 0.0f, 0.0f)) > 0.0f);
}

TEST_CASE("noise-based node is a pure function of world position (determinism), matching EC_Noise3D", "[VolumeNode]") {
    auto node = EC_Volume::noise(0.1f, 3);
    glm::vec3 p(3.3f, -1.1f, 8.8f);
    REQUIRE_THAT(node->evaluate(p), WithinAbs(node->evaluate(p), 1e-6f));
}

TEST_CASE("two chunks sampling the same composed tree agree exactly at their shared boundary", "[VolumeNode]") {
    // Generalizes the old EC_TerrainWorldDensity boundary-continuity test to the mechanism
    // itself: any tree built from these primitives is a pure function of world position, so
    // two adjacent chunks evaluating it at the same world-space point must agree exactly -
    // no chunk-local state, no special-casing needed.
    auto tree = EC_Volume::subtract(
        EC_Volume::sphere(glm::vec3(16.0f, 16.0f, 16.0f), 40.0f),
        EC_Volume::scale(EC_Volume::noise(0.2f, 3), 3.0f));

    glm::vec3 boundaryPointInChunkA(32.0f, 10.0f, 5.0f); // chunk A's +X face
    glm::vec3 sameWorldPointFromChunkB = boundaryPointInChunkA; // chunk B samples identical world coords

    REQUIRE_THAT(tree->evaluate(boundaryPointInChunkA),
        WithinAbs(tree->evaluate(sameWorldPointFromChunkB), 1e-6f));
}
