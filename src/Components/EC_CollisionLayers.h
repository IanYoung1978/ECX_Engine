#pragma once
#include <cstdint>

// Reserved collision-layer bits shared across gameplay collision and spatial/rendering
// queries (EC_BroadPhase's FrustumCheck/EntitySearch request responders). Kept separate
// from EC_DOD_Components.h since it's a shared vocabulary, not a component.
namespace CollisionLayers
{
    // Every entity that should be discoverable by a spatial/rendering query (camera
    // frustum, light influence radius) carries this bit, whether via an explicit
    // <Collider> the author OR's it into, or an auto-generated one
    // (see EC_DOD_EntityFactory::constructEntity). Auto-generated colliders use a
    // collisionMask of 0 so they never form gameplay collision pairs with anything -
    // this bit only matters for single-sided spatial discovery queries, which test a
    // candidate's collisionLayer directly and never consult its mask.
    constexpr uint32_t Renderable = 1u << 31;
}
