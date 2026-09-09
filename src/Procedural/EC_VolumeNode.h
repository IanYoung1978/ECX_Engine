#pragma once
#include <glm/glm.hpp>
#include <memory>

// A small, composable library of native volume primitives - signed-density shapes (sphere,
// box, cylinder, halfspace), noise, and CSG-style combinators (union/intersect/subtract,
// plus smooth-blended variants) - that an author composes into a tree describing an
// arbitrary 3D shape. Pure C++/glm, zero engine/terrain/game coupling, same reusability bar
// as EC_Noise3D: any game can build a cylinder habitat shell, rolling hills, a cave system,
// or anything else expressible from these primitives, without touching engine source.
//
// Convention matches EC_DensityField's own documented convention: negative = solid,
// positive = open - so any composed tree's evaluate() output drops straight into the
// existing marching-cubes pipeline unchanged. Every node is stateless and safe to evaluate
// from any thread once built (composition itself - building the tree - is expected to
// happen once, single-threaded, e.g. from a one-shot Lua script; see EC_VolumeAPI).
class EC_VolumeNode {
public:
    virtual ~EC_VolumeNode() = default;
    virtual float evaluate(const glm::vec3& p) const = 0;
};

using EC_VolumeNodePtr = std::shared_ptr<EC_VolumeNode>;

namespace EC_Volume {

// --- Sources -----------------------------------------------------------------------

EC_VolumeNodePtr constant(float value);

// Wraps EC_Noise3D::fbm3D at the given frequency - output is the classic ~[-1,1]
// gradient-noise range, same as EC_Noise3D itself; combine with Scale/Add to bias/resize.
EC_VolumeNodePtr noise(float frequency, int octaves = 3, float lacunarity = 2.0f, float persistence = 0.5f);

EC_VolumeNodePtr sphere(const glm::vec3& center, float radius);
EC_VolumeNodePtr box(const glm::vec3& center, const glm::vec3& halfExtents);

// Generalizes "plane": solid on the side `normal` points away from. A rolling-hills
// ceiling is halfspace(point-at-height, +Y) with noise subtracted; a floor cap is
// halfspace(point-at-floor, -Y).
EC_VolumeNodePtr halfspace(const glm::vec3& point, const glm::vec3& normal);

// Finite cylinder between the two axis endpoints. For an effectively-infinite shell (e.g.
// an O'Neill cylinder habitat), pass endpoints far outside the generated region.
EC_VolumeNodePtr cylinder(const glm::vec3& pointA, const glm::vec3& pointB, float radius);

// --- Numeric combinators -------------------------------------------------------------

EC_VolumeNodePtr add(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b);
EC_VolumeNodePtr scale(const EC_VolumeNodePtr& a, float factor);

// --- CSG combinators (min = union, max = intersect, max(a,-b) = subtract) ------------

EC_VolumeNodePtr unionOf(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b);
EC_VolumeNodePtr intersect(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b);
EC_VolumeNodePtr subtract(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b);

// Polynomial-smin blended variants (standard technique) - `k` controls blend radius; k=0
// degenerates to the hard CSG version above.
EC_VolumeNodePtr smoothUnion(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b, float k);
EC_VolumeNodePtr smoothIntersect(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b, float k);
EC_VolumeNodePtr smoothSubtract(const EC_VolumeNodePtr& a, const EC_VolumeNodePtr& b, float k);

// --- Transform -------------------------------------------------------------------------

EC_VolumeNodePtr translate(const EC_VolumeNodePtr& a, const glm::vec3& offset);

} // namespace EC_Volume
