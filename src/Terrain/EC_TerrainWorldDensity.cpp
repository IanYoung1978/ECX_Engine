#include "Terrain/EC_TerrainWorldDensity.h"
#include <cmath>

namespace EC_TerrainWorldDensity {

namespace {
    constexpr float kBaseHeight = 10.0f;
    constexpr float kAmplitude = 3.0f;
}

float sampleWorldDensity(const glm::vec3& worldPos)
{
    // Same rolling-hills formula as the original single-chunk verification field, just
    // evaluated at world-space X/Z instead of chunk-local coordinates - sin/cos are
    // globally continuous, so any two chunks sampling this function at the same world
    // position (their shared boundary/halo) get the identical value, with no chunk
    // ever needing to know about its neighbours' own data.
    float height = kBaseHeight
        + kAmplitude * std::sin(worldPos.x * 0.25f) * std::cos(worldPos.z * 0.2f)
        + 0.5f * kAmplitude * std::sin(worldPos.x * 0.6f + 1.3f) * std::sin(worldPos.z * 0.5f);

    return worldPos.y - height;
}

} // namespace EC_TerrainWorldDensity
