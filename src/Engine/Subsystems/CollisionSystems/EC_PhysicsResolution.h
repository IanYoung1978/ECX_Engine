#pragma once
#include "Entity/EC_DOD_Types.h"
#include "EC_PairManager.h"
#include <glm/glm.hpp>

namespace EC_PhysicsResolution
{
    void wakeIfSleeping(EntityID entity);

    glm::mat3 computeInvInertiaWorld(EntityID entity, float mass);

    // One body's state for this tick's velocity solve: real velocity/
    // angular velocity as RECORDED at the start of the solve (never
    // mutated again), inverse mass/inertia and material properties, and
    // the impulse accumulated across every contact point touching it so
    // far - not applied to real EC_DOD_Spatial until the caller's own
    // final apply step, after every contact point on every pair has
    // contributed.
    struct BodyState
    {
        glm::vec3 recordedVelocity{ 0.0f };
        glm::vec3 recordedAngVelocity{ 0.0f };
        float invMass = 0.0f;
        float restitution = 0.0f;
        float staticFriction = 0.0f;
        float kineticFriction = 0.0f;
        glm::mat3 invInertiaWorld{ 0.0f };
        glm::vec3 accumulatedLinearImpulse{ 0.0f };
        glm::vec3 accumulatedAngularImpulse{ 0.0f };
    };

    // Records entity's real current velocity, mass, inertia, and material
    // properties. Call once per body per tick, before any contact point is
    // processed.
    BodyState recordBody(EntityID entity);

    // This body's velocity ESTIMATE at a point: recordedVelocity/
    // recordedAngVelocity plus whatever's accumulated on it so far this
    // tick. Never reads or writes real EC_DOD_Spatial state.
    glm::vec3 estimatedVelocityAtPoint(const BodyState& body, const glm::vec3& r);

    // Re-applies a contact point's PREVIOUSLY CONVERGED impulse (from the
    // last time this pair was solved) into bodyA/bodyB's accumulator,
    // before any new pass runs this tick. Without this, only the CHANGE in
    // the cached impulse ever reaches real velocity - fine for the normal
    // constraint (a resting contact's required impulse is nearly the same
    // tick to tick, so the change is naturally small), but wrong for
    // kinetic friction, which is an ONGOING force: once a sliding contact's
    // cached impulse saturates the friction cone (immediately, in
    // practice), the cone clamp keeps producing the SAME total every pass,
    // the delta collapses to ~zero, and the object stops decelerating
    // while still sliding. Call once per contact point per tick, before
    // the solver passes.
    void applyWarmStart(
        BodyState& bodyA, BodyState& bodyB,
        const glm::vec3& posA, const glm::vec3& posB,
        const glm::vec3& contactPoint, const glm::vec3& normal,
        const EC_ContactImpulseCache& cache);

    // Computes this pass's incremental impulse for one contact point,
    // solving the normal (non-penetration) constraint and both tangential
    // (Coulomb friction) directions as one coupled 3x3 system - the
    // tangent basis is built from the contact normal's own geometry
    // (stable), not from relative velocity (unstable near rest, where
    // normalizing a tiny/noisy vector swings the friction direction wildly
    // pass to pass). Adds the result into bodyA/bodyB's accumulated
    // impulse and the cache's running totals (warm-started, clamped
    // non-negative for the normal impulse). Never touches real
    // EC_DOD_Spatial; the caller applies the final accumulated totals once,
    // after every pass and every contact point is done.
    void accumulateContactImpulse(
        BodyState& bodyA, BodyState& bodyB,
        const glm::vec3& posA, const glm::vec3& posB,
        const glm::vec3& contactPoint, const glm::vec3& normal,
        EC_ContactImpulseCache& cache);
}
