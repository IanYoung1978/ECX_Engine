#pragma once
#include "Entity/EC_DOD_Types.h"
#include "EC_CollisionShapes.h"
#include "EC_PairManager.h"
#include <glm/glm.hpp>

namespace EC_PhysicsResolution
{
    // Stage 1 of 2 (collision resolution). Computes the normal + friction
    // impulse at every contact point in the manifold - based on the bodies'
    // velocities as of the start of this tick, not updated between points -
    // and adds the resulting linear/angular momentum contributions to each
    // body's EC_DOD_ImpulseAccumulator. Does NOT touch EC_DOD_Spatial
    // velocity/angVelocity at all; that happens in stage 2 (see
    // EC_PhysicsSystem), which consumes every body's accumulator alongside
    // gravity and damping in one place. Entities without an EC_DOD_RigidBody
    // are treated as infinite-mass/static, so any existing scene geometry
    // acts as a solid obstacle with no changes required.
    //
    // Purely a velocity-space impulse: cancels closing velocity (with
    // restitution) at every contact point, nothing else. Penetration depth
    // is NOT folded in here - that's correctPosition's job, run once per
    // pair separately after velocity resolution. Combining a velocity-space
    // depth bias with a direct position correction double-corrects (the
    // position fix removes the overlap, but the bias-driven separating
    // velocity it also added is still there and keeps pushing the body
    // apart on top of that), which is why the two are kept fully separate.
    //
    // velA/angVelA/velB/angVelB are the velocities this call resolves
    // against - NOT read from EC_DOD_Spatial internally, so the caller
    // controls exactly what "start of tick" means. For a single isolated
    // pair that's just each body's real EC_DOD_Spatial velocity. For a body
    // touching multiple pairs at once (e.g. a cube in a stack, resting on
    // one neighbour and freshly struck by another), a single pass across
    // all pairs computed against the real, un-updated Spatial velocity
    // can't converge - the floor's support impulse doesn't "see" a fresh
    // downward hit from above still landing this same tick, so it
    // undercorrects. EC_PhysicsSystem resolves this sequentially across all
    // pairs, each pair reading every earlier pair's result so far this tick
    // (via previewVelocity below), rather than calling this with raw
    // Spatial.
    //
    // contactCache is this pair's persistent per-contact-point accumulated
    // impulse (warm starting), already prepared for this tick by exactly
    // one prior call to warmStartPair (see below) - indexed 1:1 with
    // manifold.contactPoints. This may be called several times per tick
    // (one per solver pass); each call refines the running totals already
    // in contactCache further, it does not re-seed them - re-seeding on
    // every pass would double-apply the warm start.
    void accumulateImpulses(EntityID a, EntityID b, const CollisionManifold& manifold,
        const glm::vec3& velA, const glm::vec3& angVelA,
        const glm::vec3& velB, const glm::vec3& angVelB,
        std::vector<EC_ContactImpulseCache>& contactCache);

    // Call exactly once per pair per tick, before any accumulateImpulses
    // calls for it. Matches this tick's contactPoints to last tick's cache
    // by closest position (within a small radius) and carries over each
    // matched point's normal impulse as this tick's starting guess -
    // "warm starting" - immediately applying it to both bodies'
    // EC_DOD_ImpulseAccumulator so every subsequent previewVelocity call
    // this tick (across every solver pass) reflects it. Unmatched points
    // (a genuinely new contact) start at zero, same as with no warm
    // starting at all. Friction is deliberately NOT carried over - its
    // tangent direction is derived from the current slip direction each
    // call, which can rotate tick-to-tick, so a carried-over magnitude
    // wouldn't cleanly apply to a different direction. This is what
    // actually lets a sustained contact (a resting or tumbling box)
    // converge tick-to-tick instead of re-deriving - and re-injecting
    // solver noise from - a fresh answer every single frame. Standard
    // technique in every production physics engine.
    void warmStartPair(EntityID a, EntityID b, const CollisionManifold& manifold,
        std::vector<EC_ContactImpulseCache>& contactCache);

    // Returns entity's real EC_DOD_Spatial velocity/angular velocity plus
    // whatever its EC_DOD_ImpulseAccumulator has accumulated so far this
    // tick, converted through invMass/invInertia - i.e. "the velocity this
    // body would have if step 2 applied the accumulator right now, minus
    // gravity/damping (which are step 2's job, not relevant mid-solve)".
    // Called before resolving every pair, so each pair sees every earlier
    // pair's contribution so far this tick (sequential/Gauss-Seidel, not a
    // frozen per-sweep snapshot); entities without a RigidBody (static
    // geometry) just return their Spatial velocity unchanged (always zero
    // for genuinely static geometry).
    void previewVelocity(EntityID entity, glm::vec3& outVel, glm::vec3& outAngVel);

    // Call once per pair, before accumulating its impulses, to apply sleep
    // bookkeeping: wakes a sleeping body if the other side of the pair has
    // real motion, and returns false (meaning skip accumulateImpulses/
    // correctPosition this frame) when both sides are already at rest
    // relative to each other - resolving an already-settled pair every frame
    // is exactly what re-injects the tiny numerical drift sleeping exists to
    // prevent.
    bool shouldResolve(EntityID a, EntityID b);

    // Wakes the entity if it has a sleeping RigidBody, no-op otherwise. Call
    // when a pair stops colliding (CollisionEndEvent) - a sleeping body may
    // have been resting on whatever it just separated from (e.g. knocked out
    // from under it), so losing that contact needs to re-evaluate it rather
    // than leaving it frozen mid-air with nothing left holding it up.
    void wakeIfSleeping(EntityID entity);

    // Direct positional correction: separates a and b along the manifold
    // normal by a fraction of the excess penetration (beyond slop), no
    // velocity/momentum involved. Companion to accumulateImpulses'
    // velocity-space resolution, not a replacement for it - velocity
    // resolution is rate-limited by construction (it only closes a
    // fraction of the gap per second, however stiff), so a body under
    // sustained load (e.g. still supporting the rest of a stack) settles
    // into an equilibrium at whatever depth balances that rate against the
    // load, and a fast transient impact can outrun it entirely before it
    // catches up. Direct position correction has no such rate limit -
    // called once per still-colliding pair, after velocity has already
    // been resolved, it closes most of the gap immediately regardless of
    // load or impact speed. Standard technique (every mainstream engine
    // pairs the two for this exact reason).
    //
    // Translation only - deliberately never touches orientation. A version
    // of this that also nudged orientation directly (a per-point pseudo-
    // torque split across manifold.contactPoints, intended to fix tilted
    // contacts a pure translation can't) was tried and reverted: for a
    // body in a SUSTAINED asymmetric contact (leaning against a neighbour
    // rather than resting flat), gravity regenerates the same small
    // overlap every tick, so that correction reapplied every tick too, in
    // the same direction, silently accumulating into a large rotation over
    // many seconds - entirely outside spatial.angVelocity, so the sleep
    // system (which only ever looks at velocity) had no way to see or stop
    // it. Rotation belongs entirely to the velocity solver
    // (accumulateImpulses) - if a body needs to rotate, that has to come
    // from a real, measurable angular velocity, not a position-space nudge
    // with no velocity behind it.
    void correctPosition(EntityID a, EntityID b, const CollisionManifold& manifold);

    // Diagonal inverse inertia tensor for entity's collider shape, rotated
    // into world space (sphere: 2/5*m*r^2; box: standard 1/12*m*(h^2+d^2) per
    // axis, using the OBB's current orientation - AABB colliders don't
    // rotate so use the identity orientation). Shared between stage 1
    // (computing angular impulse denominators) and stage 2 (turning
    // accumulated angular momentum into angular velocity) so both use
    // exactly the same inertia. Capsule/Cylinder/etc have no
    // collision-response dispatch entries at all (pre-existing gap), so they
    // fall back to infinite inertia (zero matrix - no rotation imparted)
    // rather than guessing a formula.
    glm::mat3 computeInvInertiaWorld(EntityID entity, float mass);
}
