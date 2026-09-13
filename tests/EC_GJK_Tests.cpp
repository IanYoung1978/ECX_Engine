// Unit tests for EC_GJK / EC_ConvexSupport (Issues #30/#29), isolated from the
// engine (no SDL/OpenGL/ECS/Lua dependency - just glm). Part of the ECX_UnitTests
// Catch2 target.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Engine/Subsystems/CollisionSystems/EC_GJK.h"
#include "Engine/Subsystems/CollisionSystems/EC_ConvexSupport.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

using Catch::Matchers::WithinAbs;

namespace {
    using namespace EC_ConvexSupport;

    EC_GJK::SupportFn sphereFn(const glm::vec3& c, float r)
    {
        return [c, r](const glm::vec3& d) { return sphereSupport(c, r, d); };
    }

    EC_GJK::SupportFn boxFn(const glm::vec3& c, const glm::vec3& he, const glm::mat3& rot)
    {
        return [c, he, rot](const glm::vec3& d) { return boxSupport(c, he, rot, d); };
    }

    EC_GJK::SupportFn capsuleFn(const glm::vec3& a, const glm::vec3& b, float r)
    {
        return [a, b, r](const glm::vec3& d) { return capsuleSupport(a, b, r, d); };
    }

    EC_GJK::SupportFn cylinderFn(const glm::vec3& a, const glm::vec3& b, float r)
    {
        return [a, b, r](const glm::vec3& d) { return cylinderSupport(a, b, r, d); };
    }

    EC_GJK::SupportFn coneFn(const glm::vec3& apex, const glm::vec3& axis, float halfAngleRad, float height)
    {
        return [apex, axis, halfAngleRad, height](const glm::vec3& d) {
            return coneSupport(apex, axis, halfAngleRad, height, d);
        };
    }

    void checkNear(const glm::vec3& actual, const glm::vec3& expected, float tolerance)
    {
        REQUIRE(glm::length(actual - expected) <= tolerance);
    }
}

TEST_CASE("GJK intersects: sphere vs sphere", "[GJK]") {
    REQUIRE(EC_GJK::intersects(sphereFn({ 0, 0, 0 }, 1.0f), sphereFn({ 1.5f, 0, 0 }, 1.0f)));
    REQUIRE_FALSE(EC_GJK::intersects(sphereFn({ 0, 0, 0 }, 1.0f), sphereFn({ 3.0f, 0, 0 }, 1.0f)));
}

TEST_CASE("GJK intersects: box vs box", "[GJK]") {
    glm::mat3 identity(1.0f);
    REQUIRE(EC_GJK::intersects(
        boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
        boxFn({ 1.5f, 0, 0 }, { 1, 1, 1 }, identity)));
    REQUIRE_FALSE(EC_GJK::intersects(
        boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
        boxFn({ 3.0f, 0, 0 }, { 1, 1, 1 }, identity)));
}

TEST_CASE("GJK intersects: OBB vs AABB", "[GJK]") {
    glm::mat3 identity(1.0f);
    glm::mat3 rot45 = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0)));
    // Box rotated 45deg around Y with half-extents (1,1,1) reaches ~1.41421 along
    // world X (cos45 + sin45). Combined with the AABB's own 1.0 half-extent, the
    // overlap threshold along X is ~2.41421.
    REQUIRE(EC_GJK::intersects(
        boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
        boxFn({ 1.8f, 0, 0 }, { 1, 1, 1 }, rot45)));
    REQUIRE_FALSE(EC_GJK::intersects(
        boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity),
        boxFn({ 3.5f, 0, 0 }, { 1, 1, 1 }, rot45)));
}

TEST_CASE("GJK intersects: cone vs sphere", "[GJK]") {
    glm::vec3 apex(0, 0, 0), axis(0, 0, 1);
    float halfAngle = glm::radians(20.0f);
    float height = 50.0f;

    SECTION("dead ahead, unobstructed") {
        REQUIRE(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height), sphereFn({ 0, 0, 10 }, 1.0f)));
    }

    SECTION("45deg off-axis, small radius - well outside a 20deg cone") {
        glm::vec3 offAxisPos = glm::vec3(std::sin(glm::radians(45.0f)), 0, std::cos(glm::radians(45.0f))) * 10.0f;
        REQUIRE_FALSE(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height), sphereFn(offAxisPos, 1.0f)));
    }

    SECTION("center outside angle but radius overlaps cone edge") {
        // Center at ~24.2deg (outside a 20deg cone) but radius 2 large enough that its
        // near edge dips inside the cone's ~3.64-radius cross-section at that depth.
        glm::vec3 partialPos(4.5f, 0, 10.0f);
        REQUIRE(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height), sphereFn(partialPos, 2.0f)));
    }
}

TEST_CASE("GJK intersects: cone vs box", "[GJK]") {
    // Reproduces the reported bug: a box whose center is outside the cone's angle
    // but whose near corner pokes inside it.
    glm::vec3 apex(0, 0, 0), axis(0, 0, 1);
    float halfAngle = glm::radians(20.0f);
    float height = 50.0f;
    glm::mat3 identity(1.0f);

    SECTION("center outside angle, near corner inside cone") {
        // Box center (5,0,10) is at atan(5/10)=26.57deg (outside 20deg), but its near
        // corner at x=5-2=3 is at atan(3/10)=16.7deg (inside 20deg).
        REQUIRE(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height),
            boxFn({ 5, 0, 10 }, { 2, 2, 2 }, identity)));
    }

    SECTION("entirely outside cone angle, including near corner") {
        // Box entirely outside even at its nearest corner (x=9, atan(9/10)=41.9deg).
        REQUIRE_FALSE(EC_GJK::intersects(coneFn(apex, axis, halfAngle, height),
            boxFn({ 10, 0, 10 }, { 1, 1, 1 }, identity)));
    }
}

TEST_CASE("GJK raycast: sphere", "[GJK][raycast]") {
    float t; glm::vec3 n;

    SECTION("hits, distance and normal are correct") {
        bool hit = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f, sphereFn({ 0, 0, 0 }, 2.0f), t, n);
        REQUIRE(hit);
        REQUIRE_THAT(t, WithinAbs(8.0f, 0.01f)); // origin -10, surface at -2
        checkNear(n, { -1, 0, 0 }, 0.01f);
    }

    SECTION("ray offset beyond radius misses") {
        bool miss = EC_GJK::raycast({ -10, 5, 0 }, { 1, 0, 0 }, 100.0f, sphereFn({ 0, 0, 0 }, 2.0f), t, n);
        REQUIRE_FALSE(miss);
    }
}

TEST_CASE("GJK raycast: AABB", "[GJK][raycast]") {
    float t; glm::vec3 n;
    glm::mat3 identity(1.0f);

    SECTION("hits, distance is correct") {
        bool hit = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f,
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity), t, n);
        REQUIRE(hit);
        REQUIRE_THAT(t, WithinAbs(9.0f, 0.01f)); // origin -10, face at -1
    }

    SECTION("ray offset beyond extents misses") {
        bool miss = EC_GJK::raycast({ -10, 5, 0 }, { 1, 0, 0 }, 100.0f,
            boxFn({ 0, 0, 0 }, { 1, 1, 1 }, identity), t, n);
        REQUIRE_FALSE(miss);
    }
}

TEST_CASE("GJK raycast: OBB accounts for rotated silhouette", "[GJK][raycast]") {
    float t; glm::vec3 n;
    glm::mat3 rot45 = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0)));
    bool hit = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f,
        boxFn({ 0, 0, 0 }, { 1, 1, 1 }, rot45), t, n);
    REQUIRE(hit);
    // Reach along X for a 45deg-rotated unit-half-extent box is cos45+sin45 ~= 1.41421.
    REQUIRE_THAT(t, WithinAbs(10.0f - 1.41421f, 0.02f));
}

TEST_CASE("GJK raycast: capsule body and cap", "[GJK][raycast]") {
    float t; glm::vec3 n;
    glm::vec3 a(0, -2, 0), b(0, 2, 0);

    SECTION("through the cylindrical body") {
        bool hitBody = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f, capsuleFn(a, b, 1.0f), t, n);
        REQUIRE(hitBody);
        REQUIRE_THAT(t, WithinAbs(9.0f, 0.01f));
    }

    SECTION("through the rounded top cap only") {
        // Capsule axis runs along Y, so "above the straight segment" means offsetting
        // Y past b.y=2, not Z.
        bool hitCap = EC_GJK::raycast({ -10, 2.5f, 0 }, { 1, 0, 0 }, 100.0f, capsuleFn(a, b, 1.0f), t, n);
        REQUIRE(hitCap);
        REQUIRE_THAT(t, WithinAbs(9.134f, 0.05f));
    }
}

TEST_CASE("GJK raycast: cylinder flat cap and body", "[GJK][raycast]") {
    float t; glm::vec3 n;
    glm::vec3 a(0, -2, 0), b(0, 2, 0);

    SECTION("straight up through the flat bottom cap") {
        bool hitCap = EC_GJK::raycast({ 0, -10, 0 }, { 0, 1, 0 }, 100.0f, cylinderFn(a, b, 1.0f), t, n);
        REQUIRE(hitCap);
        REQUIRE_THAT(t, WithinAbs(8.0f, 0.01f));
        checkNear(n, { 0, -1, 0 }, 0.01f);
    }

    SECTION("through the round side") {
        bool hitBody = EC_GJK::raycast({ -10, 0, 0 }, { 1, 0, 0 }, 100.0f, cylinderFn(a, b, 1.0f), t, n);
        REQUIRE(hitBody);
        REQUIRE_THAT(t, WithinAbs(9.0f, 0.01f));
    }
}
