#pragma once
#include "Entity/EC_DOD_Types.h"
#include "EC_PairManager.h"
#include <glm/glm.hpp>

namespace EC_PhysicsResolution
{
    void wakeIfSleeping(EntityID entity);

    glm::mat3 computeInvInertiaWorld(EntityID entity, float mass);

    // One body's state for this tick's velocity solve: velocity recorded at the start (never mutated),
    // inverse mass/inertia, material properties, and the impulse accumulated so far. Applied to Spatial
    // only in the caller's final apply step.
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

    // Re-applies a contact's previously converged impulse before this tick's passes. Without it only
    // the change reaches velocity, so kinetic friction (an ongoing force) vanishes once its cached
    // impulse saturates the cone. Call once per contact point per tick, before the solver passes.
    void applyWarmStart(
        BodyState& bodyA, BodyState& bodyB,
        const glm::vec3& posA, const glm::vec3& posB,
        const glm::vec3& contactPoint, const glm::vec3& normal,
        const EC_ContactImpulseCache& cache);

    // One solver pass for a contact point: normal and both friction directions as one coupled 3x3
    // system, with the tangent basis built from the contact normal (stable near rest). Adds into the
    // bodies' accumulators and the cache's warm-started totals; never touches Spatial.
    void accumulateContactImpulse(
        BodyState& bodyA, BodyState& bodyB,
        const glm::vec3& posA, const glm::vec3& posB,
        const glm::vec3& contactPoint, const glm::vec3& normal,
        EC_ContactImpulseCache& cache);
}
