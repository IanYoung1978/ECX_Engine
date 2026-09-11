// Unit tests for EC_Noise3D - pure C++/glm, no engine dependency. This is reusable
// procedural-generation infrastructure (not terrain-specific), so what matters here is
// its own contract: determinism, continuity, and a sane output range - not any particular
// game's use of it.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Procedural/EC_Noise3D.h"

using Catch::Matchers::WithinAbs;

TEST_CASE("noise3D is a deterministic pure function of its input", "[Noise3D]") {
    glm::vec3 p(3.7f, -1.2f, 8.4f);
    REQUIRE(EC_Noise3D::noise3D(p) == EC_Noise3D::noise3D(p));
}

TEST_CASE("noise3D stays within the classic gradient-noise range", "[Noise3D]") {
    for (float x = -5.0f; x <= 5.0f; x += 0.37f) {
        for (float y = -5.0f; y <= 5.0f; y += 0.53f) {
            float n = EC_Noise3D::noise3D(glm::vec3(x, y, 1.234f));
            REQUIRE(n >= -1.5f);
            REQUIRE(n <= 1.5f);
        }
    }
}

TEST_CASE("noise3D is continuous - small input steps produce small output steps", "[Noise3D]") {
    glm::vec3 p(2.0f, 5.0f, -3.0f);
    float base = EC_Noise3D::noise3D(p);
    float stepped = EC_Noise3D::noise3D(p + glm::vec3(0.001f, 0.0f, 0.0f));
    REQUIRE_THAT(stepped, WithinAbs(base, 0.05f));
}

TEST_CASE("noise3D is not just a constant (varies across space)", "[Noise3D]") {
    // Deliberately non-integer inputs - classic gradient noise is exactly 0 at every
    // integer lattice point by construction (the fractional part, and so the gradient
    // dot product, is zero there), so integer coordinates would trivially collide.
    float a = EC_Noise3D::noise3D(glm::vec3(0.3f, 0.7f, 0.2f));
    float b = EC_Noise3D::noise3D(glm::vec3(100.4f, 37.1f, -62.6f));
    REQUIRE(a != b);
}

TEST_CASE("fbm3D matches a single noise3D call at one octave", "[Noise3D]") {
    glm::vec3 p(1.5f, 2.5f, 3.5f);
    REQUIRE_THAT(EC_Noise3D::fbm3D(p, 1), WithinAbs(EC_Noise3D::noise3D(p), 1e-5f));
}

TEST_CASE("fbm3D stays roughly in range regardless of octave count", "[Noise3D]") {
    glm::vec3 p(4.2f, -7.1f, 0.6f);
    for (int octaves = 1; octaves <= 6; octaves++) {
        float v = EC_Noise3D::fbm3D(p, octaves);
        REQUIRE(v >= -1.5f);
        REQUIRE(v <= 1.5f);
    }
}
