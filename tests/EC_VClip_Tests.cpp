// Unit tests for EC_VClip::generateContactPoints - given two already-overlapping
// OBBs plus SAT's normal/depth, verifies it picks a plausible feature pair and
// produces the right contact-point count (1 for vertex/edge, up to 4 for a
// genuine face-face rest). Pure glm + shape structs, no engine dependency.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Engine/Subsystems/CollisionSystems/EC_VClip.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("generateContactPoints resolves a flat face-face rest to a 4-point manifold", "[VClip]") {
    // Box A: a large flat "floor" slab, box B: a unit cube sitting on top of it,
    // both axis-aligned - classic stable-rest configuration.
    OBB floorBox{ glm::vec3(0.0f), glm::vec3(10.0f, 0.5f, 10.0f), glm::mat3(1.0f) };
    OBB cube{ glm::vec3(0.0f), glm::vec3(0.5f), glm::mat3(1.0f) };

    // Floor top face is at y=0.5, cube center at y=0.98 (bottom at 0.48):
    // slight overlap of 0.02 along Y, matching what SAT would report.
    glm::vec3 posFloor(0.0f);
    glm::vec3 posCube(0.0f, 0.98f, 0.0f);
    glm::vec3 normal(0.0f, 1.0f, 0.0f); // floor -> cube
    float penetration = 0.02f;

    CollisionManifold manifold;
    EC_VClip::generateContactPoints(floorBox, posFloor, cube, posCube, normal, penetration, manifold);

    REQUIRE(manifold.contactPoints.size() == 4);
    // Points land on the incident (cube) face, i.e. the cube's actual bottom
    // face at y = 0.98 - 0.5 = 0.48, not the reference (floor) face at y = 0.5.
    for (const auto& p : manifold.contactPoints) {
        REQUIRE_THAT(p.y, WithinAbs(0.48f, 1e-2f));
    }
}

TEST_CASE("generateContactPoints resolves a corner-down tilted box to a single contact point", "[VClip]") {
    OBB floorBox{ glm::vec3(0.0f), glm::vec3(10.0f, 0.5f, 10.0f), glm::mat3(1.0f) };

    // Classic "cube balanced on a vertex" orientation: Rx(-35.264) * Rz(45),
    // applied Z-first, sends local corner (-he,-he,-he) to straight down (-Y).
    glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(-35.264f), glm::vec3(1.0f, 0.0f, 0.0f));
    rot = glm::rotate(rot, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    OBB cube{ glm::vec3(0.0f), glm::vec3(0.5f), glm::mat3(rot) };

    // Lowest corner of the tilted cube is at distance he*sqrt(3) ~= 0.866 from center.
    // Place it just barely penetrating the floor's top face (y = 0.5).
    glm::vec3 posFloor(0.0f);
    glm::vec3 posCube(0.0f, 0.5f + 0.866f - 0.02f, 0.0f);
    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    float penetration = 0.02f;

    CollisionManifold manifold;
    EC_VClip::generateContactPoints(floorBox, posFloor, cube, posCube, normal, penetration, manifold);

    REQUIRE(manifold.contactPoints.size() == 1);
}
