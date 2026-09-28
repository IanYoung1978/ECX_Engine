#pragma once
#include <cstdint>

// Reserved collision-layer bits shared across gameplay collision and spatial/rendering
// queries (EC_BroadPhase's FrustumCheck/EntitySearch request responders). Kept separate
// from EC_DOD_Components.h since it's a shared vocabulary, not a component.
namespace CollisionLayers
{
    // Carried by every entity a spatial/rendering query should find (camera frustum, light radius),
    // via an authored <Collider> or an auto-generated one. Auto-generated colliders use collisionMask 0,
    // so they never form gameplay pairs; queries test the layer bit directly and ignore the mask.
    constexpr uint32_t Renderable = 1u << 31;
}
