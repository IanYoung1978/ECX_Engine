#pragma once
#include <glm/glm.hpp>

// Deterministic 3D gradient noise, pure C++/glm. noise3D returns roughly [-1, 1]; fbm3D layers
// `octaves` copies (frequency x lacunarity, amplitude x persistence), normalized to the same range.
namespace EC_Noise3D {

float noise3D(const glm::vec3& p);

float fbm3D(const glm::vec3& p, int octaves, float lacunarity = 2.0f, float persistence = 0.5f);

} // namespace EC_Noise3D
