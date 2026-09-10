// Unit tests for the pure narrow-phase shape-vs-shape tests (Tier 1 of the
// testability review): no engine/entity/GL dependency, only glm + the shape
// structs themselves.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

TEST_CASE("CapsuleVsSphere detects overlap against the capsule's segment, not just its endpoints", "[CollisionChecks][Capsule]") {
    // Vertical capsule from y=-2 to y=2, radius 1 - a sphere touching the MIDDLE of the
    // segment (not near either endpoint) must still register, proving this tests the
    // whole segment rather than treating the capsule as just two spheres at its ends.
    Capsule capsule{ glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f, 2.0f, 0.0f), 1.0f };
    Sphere sphere{ glm::vec3(0.0f), 1.0f };
    CollisionManifold manifold;

    SECTION("sphere overlapping the segment's midpoint collides") {
        bool hit = EC_CollisionChecks::CapsuleVsSphere(capsule, glm::vec3(0.0f), sphere, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.5f, kTol));
    }

    SECTION("sphere near an endpoint (beyond the segment) collides via the rounded cap") {
        bool hit = EC_CollisionChecks::CapsuleVsSphere(capsule, glm::vec3(0.0f), sphere, glm::vec3(0.0f, 3.5f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.y, WithinAbs(1.0f, kTol));
    }

    SECTION("sphere far from the whole capsule does not collide") {
        bool hit = EC_CollisionChecks::CapsuleVsSphere(capsule, glm::vec3(0.0f), sphere, glm::vec3(10.0f, 0.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit);
    }
}

TEST_CASE("CapsuleVsAABB detects overlap against the nearest point on the segment", "[CollisionChecks][Capsule]") {
    Capsule capsule{ glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f, 2.0f, 0.0f), 1.0f };
    AABB box{ glm::vec3(-1.0f), glm::vec3(1.0f) };
    CollisionManifold manifold;

    SECTION("box overlapping the capsule's side collides") {
        bool hit = EC_CollisionChecks::CapsuleVsAABB(capsule, glm::vec3(0.0f), box, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
    }

    SECTION("box far from the capsule does not collide") {
        bool hit = EC_CollisionChecks::CapsuleVsAABB(capsule, glm::vec3(0.0f), box, glm::vec3(20.0f, 0.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit);
    }
}

TEST_CASE("CapsuleVsOBB matches CapsuleVsAABB for an identity-oriented box", "[CollisionChecks][Capsule]") {
    Capsule capsule{ glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f, 2.0f, 0.0f), 1.0f };
    OBB obb{ glm::vec3(0.0f), glm::vec3(1.0f), glm::mat3(1.0f) };
    CollisionManifold manifold;

    SECTION("box overlapping the capsule's side collides") {
        bool hit = EC_CollisionChecks::CapsuleVsOBB(capsule, glm::vec3(0.0f), obb, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
    }

    SECTION("rotating the box 90 degrees around Y still collides the same way (square cross-section)") {
        glm::mat3 rotated = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        OBB rotatedObb{ glm::vec3(0.0f), glm::vec3(1.0f), rotated };
        bool hit = EC_CollisionChecks::CapsuleVsOBB(capsule, glm::vec3(0.0f), rotatedObb, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
    }

    SECTION("box far from the capsule does not collide") {
        bool hit = EC_CollisionChecks::CapsuleVsOBB(capsule, glm::vec3(0.0f), obb, glm::vec3(20.0f, 0.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit);
    }
}

TEST_CASE("CapsuleVsMesh detects overlap against the real triangle surface, not just a bounding box", "[CollisionChecks][Capsule]") {
    // A flat quad (two triangles) spanning x/z in [-5,5] at y=0, normal +Y - matches
    // EC_RayIntersection_Tests.cpp's makeFlatQuad shape for consistency across the two
    // files' mesh-testing conventions.
    std::vector<glm::vec3> positions = {
        glm::vec3(-5.0f, 0.0f, -5.0f),
        glm::vec3(5.0f, 0.0f, -5.0f),
        glm::vec3(5.0f, 0.0f, 5.0f),
        glm::vec3(-5.0f, 0.0f, 5.0f),
    };
    std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
    // Per-vertex gradient normals, as EC_MarchingCubesMesher would emit for a real flat
    // surface with open space above (matches this quad's stated "normal +Y") - CapsuleVsMesh
    // now derives its contact normal from these directly rather than from the triangles'
    // vertex winding (see EC_CollisionChecks.h's CapsuleVsMesh comment for why: a winding
    // parity bug in the mesher can flip a cross-product-derived normal even on a flat
    // surface, but the gradient normals don't depend on winding at all).
    std::vector<glm::vec3> normals = {
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
    };

    // Vertical capsule, segment bottom at y=0.3, radius 0.5 - its rounded cap overlaps
    // the quad at y=0 by 0.2 (0.5 - 0.3).
    Capsule capsule{ glm::vec3(0.0f, 0.3f, 0.0f), glm::vec3(0.0f, 2.3f, 0.0f), 0.5f };
    CollisionManifold manifold;

    SECTION("capsule resting close enough to the surface collides, normal points up") {
        bool hit = EC_CollisionChecks::CapsuleVsMesh(capsule, glm::vec3(0.0f), positions, normals, indices, glm::vec3(0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.y, WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.2f, kTol));
    }

    SECTION("capsule far above the surface does not collide") {
        Capsule highCapsule{ glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(0.0f, 12.0f, 0.0f), 0.5f };
        bool hit = EC_CollisionChecks::CapsuleVsMesh(highCapsule, glm::vec3(0.0f), positions, normals, indices, glm::vec3(0.0f), manifold);
        REQUIRE_FALSE(hit);
    }

    SECTION("capsule beside the quad's edge (outside its footprint) does not collide") {
        Capsule sideCapsule{ glm::vec3(20.0f, 1.0f, 0.0f), glm::vec3(20.0f, 3.0f, 0.0f), 0.5f };
        bool hit = EC_CollisionChecks::CapsuleVsMesh(sideCapsule, glm::vec3(0.0f), positions, normals, indices, glm::vec3(0.0f), manifold);
        REQUIRE_FALSE(hit);
    }

    SECTION("mesh offset by meshPos is honoured") {
        bool hit = EC_CollisionChecks::CapsuleVsMesh(capsule, glm::vec3(0.0f), positions, normals, indices, glm::vec3(0.0f, -1.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit); // quad moved down out of range
    }
}

TEST_CASE("CapsuleVsCapsule detects overlap between two segments, self-orienting A->B", "[CollisionChecks][Capsule]") {
    Capsule a{ glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f, 2.0f, 0.0f), 1.0f };
    Capsule b{ glm::vec3(0.0f, -2.0f, 0.0f), glm::vec3(0.0f, 2.0f, 0.0f), 1.0f };
    CollisionManifold manifold;

    SECTION("parallel overlapping capsules collide, normal points A->B") {
        bool hit = EC_CollisionChecks::CapsuleVsCapsule(a, glm::vec3(0.0f), b, glm::vec3(1.5f, 0.0f, 0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.x, WithinAbs(1.0f, kTol));
        REQUIRE_THAT(manifold.penetrationDepth, WithinAbs(0.5f, kTol));
    }

    SECTION("perpendicular crossing capsules (an X shape) collide at their crossing point") {
        Capsule crossB{ glm::vec3(-2.0f, 0.0f, 1.5f), glm::vec3(2.0f, 0.0f, 1.5f), 1.0f };
        bool hit = EC_CollisionChecks::CapsuleVsCapsule(a, glm::vec3(0.0f), crossB, glm::vec3(0.0f), manifold);
        REQUIRE(hit);
        REQUIRE_THAT(manifold.contactNormal.z, WithinAbs(1.0f, kTol));
    }

    SECTION("far-apart capsules do not collide") {
        bool hit = EC_CollisionChecks::CapsuleVsCapsule(a, glm::vec3(0.0f), b, glm::vec3(20.0f, 0.0f, 0.0f), manifold);
        REQUIRE_FALSE(hit);
    }
}
