// Unit tests for the pure narrow-phase shape-vs-shape tests (Tier 1 of the
// testability review): no engine/entity/GL dependency, only glm + the shape
// structs themselves.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Engine/Subsystems/CollisionSystems/EC_CollisionChecks.h"

using Catch::Matchers::WithinAbs;

namespace {
    constexpr float kTol = 1e-4f;
}

TEST_CASE("SphereVsSphere detects overlap and reports separation-based manifold", "[CollisionChecks][Sphere]") {
    Sphere a{ glm::vec3(0.0f), 1.0f };
    Sphere b{ glm::vec3(0.0f), 1.0f };
    CollisionManifold manifold;

    SECTION("overlapping spheres collide, normal points A->B, penetration positive") {
        bool hit = EC_CollisionChecks::SphereVsSphere(a, glm::vec3(0.0f), b, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.5f, kTol));
        REQUIRE(manifold.contactPoints.size() == 1);
    }

    SECTION("spheres exactly touching (zero penetration) still register as colliding") {
        bool hit = EC_CollisionChecks::SphereVsSphere(a, glm::vec3(0.0f), b, glm::vec3(2.0f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.0f, kTol));
    }

    SECTION("spheres far apart do not collide") {
        bool hit = EC_CollisionChecks::SphereVsSphere(a, glm::vec3(0.0f), b, glm::vec3(5.0f, 0.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit);
    }
}

TEST_CASE("AABBVsAABB picks the minimum-penetration axis and orients A->B", "[CollisionChecks][AABB]") {
    AABB a{ glm::vec3(-1.0f), glm::vec3(1.0f) };
    AABB b{ glm::vec3(-1.0f), glm::vec3(1.0f) };
    CollisionManifold manifold;

    SECTION("shallow overlap on X picks X as the separating axis") {
        // A at origin, B shifted 1.5 on X: overlap on X = 0.5, Y/Z fully overlap (2.0)
        bool hit = EC_CollisionChecks::AABBVsAABB(a, glm::vec3(0.0f), b, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.5f, kTol));
        REQUIRE(manifold.contactPoints.size() == 4);
    }

    SECTION("shallow overlap on Y picks Y and produces a horizontal 4-point face") {
        bool hit = EC_CollisionChecks::AABBVsAABB(a, glm::vec3(0.0f), b, glm::vec3(0.0f, 1.5f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.y, WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.5f, kTol));
    }

    SECTION("non-overlapping boxes do not collide") {
        bool hit = EC_CollisionChecks::AABBVsAABB(a, glm::vec3(0.0f), b, glm::vec3(10.0f, 0.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit);
    }
}

TEST_CASE("SphereVsAABB reports the AABB surface point closest to the sphere center", "[CollisionChecks][SphereAABB]") {
    AABB box{ glm::vec3(-1.0f), glm::vec3(1.0f) };
    CollisionManifold manifold;

    SECTION("sphere overlapping a face") {
        Sphere s{ glm::vec3(0.0f), 0.5f };
        bool hit = EC_CollisionChecks::SphereVsAABB(s, glm::vec3(1.3f, 0.0f, 0.0f), box, glm::vec3(0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactPoints[0].x, WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
    }

    SECTION("sphere fully outside the AABB's influence does not collide") {
        Sphere s{ glm::vec3(0.0f), 0.5f };
        bool hit = EC_CollisionChecks::SphereVsAABB(s, glm::vec3(5.0f, 0.0f, 0.0f), box, glm::vec3(0.0f), manifold);
        REQUIRE_FALSE(hit);
    }

    SECTION("sphere center inside the box still resolves (zero-distance fallback normal)") {
        Sphere s{ glm::vec3(0.0f), 0.5f };
        bool hit = EC_CollisionChecks::SphereVsAABB(s, glm::vec3(0.0f), box, glm::vec3(0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.y, WithinAbs(1.0f, kTol));
    }
}

TEST_CASE("OBBVsOBB axis-aligned boxes behave like AABBVsAABB (identity orientation)", "[CollisionChecks][OBB]") {
    OBB a{ glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3(1.0f) };
    OBB b{ glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3(1.0f) };
    CollisionManifold manifold;

    SECTION("shallow overlap along X separates on X with correct depth") {
        bool hit = EC_CollisionChecks::OBBVsOBB(a, glm::vec3(0.0f), b, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(std::abs(manifold.contactNormal.x), WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.5f, kTol));
    }

    SECTION("separated boxes report no collision") {
        bool hit = EC_CollisionChecks::OBBVsOBB(a, glm::vec3(0.0f), b, glm::vec3(10.0f, 0.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit);
    }
}

TEST_CASE("FrustumVsAABB conservative sphere-bound test", "[CollisionChecks][Frustum]") {
    Frustum f{};
    f.position = glm::vec3(0.0f);
    f.direction = glm::vec3(0.0f, 0.0f, -1.0f);
    f.up = glm::vec3(0.0f, 1.0f, 0.0f);
    f.right = glm::vec3(1.0f, 0.0f, 0.0f);
    f.nearPlane = 0.1f;
    f.farPlane = 100.0f;
    f.fov = 60.0f;
    f.aspectRatio = 16.0f / 9.0f;

    AABB box{ glm::vec3(-1.0f), glm::vec3(1.0f) };

    SECTION("box near the frustum center is considered visible") {
        bool visible = EC_CollisionChecks::FrustumVsAABB(f, glm::vec3(0.0f), box, glm::vec3(0.0f, 0.0f, -50.0f));
        REQUIRE(visible);
    }

    SECTION("box far outside the frustum's bounding sphere is culled") {
        bool visible = EC_CollisionChecks::FrustumVsAABB(f, glm::vec3(0.0f), box, glm::vec3(0.0f, 0.0f, 10000.0f));
        REQUIRE_FALSE(visible);
    }
}
