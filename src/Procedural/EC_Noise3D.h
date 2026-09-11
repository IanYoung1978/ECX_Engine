#pragma once
#include <glm/glm.hpp>

// Deterministic 3D gradient (Perlin-style) noise - pure C++/glm, zero engine dependency,
// same testability bar as EC_MarchingCubesMesher/EC_TerrainWorldDensity. Deliberately
// generic and NOT terrain-specific: any density function (terrain, caves, asteroids,
// blobby rocks, whatever the next game needs) can call noise3D/fbm3D directly - this is
// the reusable "tech" itself, not glued to any one game's shape.
//
// noise3D returns values in approximately [-1, 1] (classic gradient-noise range, not
// tightly bounded). fbm3D layers `octaves` copies at increasing frequency (each step
// scaled by `lacunarity`) and decreasing amplitude (each step scaled by `persistence`),
// normalized so the result stays in roughly the same [-1, 1] range regardless of octave
// count - the standard fractal-noise recipe for natural-looking bumps/detail at multiple
// scales.
namespace EC_Noise3D {

float noise3D(const glm::vec3& p);

float fbm3D(const glm::vec3& p, int octaves, float lacunarity = 2.0f, float persistence = 0.5f);

} // namespace EC_Noise3D
