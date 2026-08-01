#include "Engine/Subsystems/EC_PhysicsSystem.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include "Engine/Subsystems/CollisionSystems/EC_PhysicsResolution.h"
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"
#include "Engine/Subsystems/CollisionSystems/EC_CollisionShapes.h"
#include <glm/gtc/matrix_transform.hpp>
#include "Logging/ECX_Logging.h"

EC_PhysicsSystem::EC_PhysicsSystem() {}
EC_PhysicsSystem::~EC_PhysicsSystem() {}
void EC_PhysicsSystem::init(ECXMessenger& messenger, EC_Game& game) {}

namespace {
    constexpr glm::vec3 kGravity(0.0f, -9.8f, 0.0f);
    // Below these speeds, a body is considered "at rest" for sleep purposes.
    constexpr float kSleepLinearThreshold = 0.05f;
    constexpr float kSleepAngularThreshold = 0.01f;
    // How long it must stay below threshold before actually going to sleep -
    // avoids putting a body to sleep during a brief momentary lull. A body
    // sitting just past an unstable equilibrium (e.g. supporting another
    // body with its combined centre of mass no longer over its own base)
    // has torque roughly proportional to sin(tilt), so its angular
    // acceleration right at the start of a topple is genuinely tiny - it
    // can sit under kSleepAngularThreshold for a while before visibly
    // accelerating. A short window here reads that as "at rest" and freezes
    // it mid-topple - and once asleep, gravity is skipped for it entirely
    // (see the isSleeping check below), so it never resumes falling even
    // though the configuration was never actually stable. 1 full second
    // (60 frames at the engine's fixed 60Hz) gives a slow-building topple
    // enough real time to build up past the threshold before being frozen;
    // Box2D's own default (b2_timeToSleep) is 0.5s for comparison.
    constexpr float kTimeToSleep = 60.0f / 60.0f;
}

void EC_PhysicsSystem::update(const float& deltaTimeS, EC_Game& game) {
    auto& manager = EC_DOD_EntityManager::getInstance();

    // --- Step 1: pairs only. Every pair the collision system found
    // colliding this tick contributes a normal + friction impulse (per
    // contact point in its cached manifold) into each body's
    // EC_DOD_ImpulseAccumulator. Nothing else happens here - no gravity,
    // no damping, no integration; those belong to step 2 below.
    //
    // Resolved sequentially (Gauss-Seidel) across every pair, several
    // passes per tick, and warm-started from last tick's result (see
    // EC_PhysicsResolution::accumulateImpulses/previewVelocity). Each
    // pair's resolution reads every earlier pair's contribution so far
    // this tick via previewVelocity (which reads the live, continuously-
    // updated accumulator) - so a body touching multiple simultaneous
    // contacts converges properly within the tick: e.g. a cube resting on
    // the floor while a falling neighbour lands on it the same tick - the
    // floor's support impulse now sees that fresh downward hit instead of
    // resolving against stale velocity. Combined with warm starting
    // (each contact's accumulated impulse persists tick-to-tick instead of
    // restarting at zero every frame), this is the standard sequential-
    // impulse architecture every production physics engine uses.
    if (m_PairManager) {
        // Warm start every still-colliding, resolvable pair exactly once
        // per tick, before the multi-pass solve below - matches this
        // tick's contact points to last tick's cache and seeds the real
        // accumulator with each one's carried-over impulse, so every
        // solver pass (and every other pair's previewVelocity) sees it as
        // this tick's starting guess. Must run to completion here, not
        // inside the pass loop, or it would double-apply on every pass.
        for (EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
            if (!pair.m_Colliding) continue;
            if (!EC_PhysicsResolution::shouldResolve(pair.body_A, pair.body_B)) continue;

            CollisionManifold manifold;
            manifold.contactPoints = pair.m_CollisionPoints;
            manifold.contactNormal = pair.m_ContactNormal;
            manifold.penetrationDepth = pair.m_PenetrationDepth;
            EC_PhysicsResolution::warmStartPair(pair.body_A, pair.body_B, manifold, pair.m_ContactCache);
        }

        constexpr int kSolverPasses = 4;
        for (int pass = 0; pass < kSolverPasses; pass++) {
            for (EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
                if (!pair.m_Colliding) continue;
                if (!EC_PhysicsResolution::shouldResolve(pair.body_A, pair.body_B)) continue;

                CollisionManifold manifold;
                manifold.contactPoints = pair.m_CollisionPoints;
                manifold.contactNormal = pair.m_ContactNormal;
                manifold.penetrationDepth = pair.m_PenetrationDepth;

                glm::vec3 velA, angVelA, velB, angVelB;
                EC_PhysicsResolution::previewVelocity(pair.body_A, velA, angVelA);
                EC_PhysicsResolution::previewVelocity(pair.body_B, velB, angVelB);

                EC_PhysicsResolution::accumulateImpulses(pair.body_A, pair.body_B, manifold,
                    velA, angVelA, velB, angVelB, pair.m_ContactCache);
            }
        }

        // --- Step 1b: direct positional correction, once per still-
        // colliding pair, after velocity has converged above. The velocity
        // bias inside accumulateImpulses is rate-limited by construction -
        // a body under sustained load (e.g. still supporting the rest of a
        // stack) settles at whatever depth balances that rate against the
        // load, and a fast impact can outrun it before it catches up. This
        // has no such limit: it closes most of the gap immediately,
        // regardless of load or impact speed. Runs once (not per sweep) -
        // it isn't part of the velocity solve's convergence, and repeating
        // it would just overshoot. ---
        for (const EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
            if (!pair.m_Colliding) continue;
            if (!EC_PhysicsResolution::shouldResolve(pair.body_A, pair.body_B)) continue;

            CollisionManifold manifold;
            manifold.contactPoints = pair.m_CollisionPoints;
            manifold.contactNormal = pair.m_ContactNormal;
            manifold.penetrationDepth = pair.m_PenetrationDepth;
            EC_PhysicsResolution::correctPosition(pair.body_A, pair.body_B, manifold);
        }
    }

    // --- Step 2: apply. Every awake, non-static rigid body consumes
    // gravity + whatever step 1 (or a prior tick's leftover, though there
    // shouldn't be any) cached in its accumulator, then integrates. ---
    auto* rbArray = manager.getComponentArray<EC_DOD_RigidBody>();
    if (!rbArray) return;

    std::shared_lock lock(rbArray->getMutex());
    auto& rigidBodies = rbArray->getData();

    for (size_t i = 0; i < rigidBodies.size(); i++) {
        auto& rb = rigidBodies[i];
        EntityID entity = rbArray->getEntity(i);

        if (rb.isStatic) continue;

        if (!manager.hasComponent<EC_DOD_Spatial>(entity)) continue;
        auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);

        if (rb.isSleeping) {
            continue; // frozen; woken by EC_PhysicsResolution when disturbed
        }

        const float invMass = (rb.mass > 1e-6f) ? (1.0f / rb.mass) : 0.0f;

        // --- Gravity: a plain linear force, applied as a momentum
        // contribution (mass * g * dt) same as any other impulse source. ---
        glm::vec3 deltaLinearMomentum = kGravity * rb.mass * deltaTimeS;
        glm::vec3 deltaAngularMomentum(0.0f);

        // --- Cached impulses from this tick's collision resolution (step 1
        // above, warm-started and converged across several solver passes). ---
        if (manager.hasComponent<EC_DOD_ImpulseAccumulator>(entity)) {
            auto& accum = manager.getComponent<EC_DOD_ImpulseAccumulator>(entity);
            deltaLinearMomentum += accum.deltaLinearMomentum;
            deltaAngularMomentum += accum.deltaAngularMomentum;
            accum.deltaLinearMomentum = glm::vec3(0.0f);
            accum.deltaAngularMomentum = glm::vec3(0.0f);
        }

        // --- Apply: linear first, then angular ---
        spatial.velocity += deltaLinearMomentum * invMass;

        const glm::mat3 invInertiaWorld = EC_PhysicsResolution::computeInvInertiaWorld(entity, rb.mass);
        spatial.angVelocity += invInertiaWorld * deltaAngularMomentum;

        // --- Damping: a per-second fractional velocity decay, independent
        // of contact - models drag/internal energy loss. Distinct from
        // friction (which only acts at a contact point) and from the
        // impulse-based forces above. ---
        spatial.velocity *= std::max(0.0f, 1.0f - rb.linearDamping * deltaTimeS);
        spatial.angVelocity *= std::max(0.0f, 1.0f - rb.angularDamping * deltaTimeS);

        // --- Sleep bookkeeping, now that this tick's velocity is final ---
        const bool atRest =
            glm::dot(spatial.velocity, spatial.velocity) < kSleepLinearThreshold * kSleepLinearThreshold &&
            glm::dot(spatial.angVelocity, spatial.angVelocity) < kSleepAngularThreshold * kSleepAngularThreshold;
        rb.sleepTimer = atRest ? (rb.sleepTimer + deltaTimeS) : 0.0f;
        if (rb.sleepTimer >= kTimeToSleep) {
            rb.isSleeping = true;
            spatial.velocity = glm::vec3(0.0f);
            spatial.angVelocity = glm::vec3(0.0f);
            continue;
        }

        // --- Integrate position/orientation from the final velocity ---
        spatial.position += spatial.velocity * deltaTimeS;
        spatial.orientation += spatial.angVelocity * deltaTimeS;

        // Rigid bodies can tumble around any axis, so they need a real
        // 3-axis rotation - built the exact same way
        // EC_TransformSystem::buildLocal composes the render matrix, so an
        // OBB collider's orientation always matches what's rendered.
        glm::mat4 rot(1.0f);
        rot = glm::rotate(rot, spatial.orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        rot = glm::rotate(rot, spatial.orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        rot = glm::rotate(rot, spatial.orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));

        spatial.direction = glm::normalize(glm::vec3(rot * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
        spatial.up = glm::normalize(glm::vec3(rot * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
        spatial.right = glm::normalize(glm::vec3(rot * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
    }

    // Total system mechanical energy (kinetic + gravitational potential, not
    // just kinetic - KE alone legitimately grows anytime something is simply
    // falling, so it isn't a useful invariant on its own) across every awake
    // dynamic body. Should only ever decrease (friction/damping/inelastic
    // impacts dissipate it) or drop sharply when a body goes to sleep
    // (excluded from the sum) - a sustained increase means the solver is
    // injecting energy it shouldn't. Off by default; enable via
    // EngineConfig.xml's <Physics><Debug><LogEnergy>true</LogEnergy>.
    if (m_LogEnergy) {
        static int s_EnergyTick = 0;
        s_EnergyTick++;
        float totalEnergy = 0.0f;
        for (size_t i = 0; i < rigidBodies.size(); i++) {
            auto& rb = rigidBodies[i];
            if (rb.isStatic || rb.isSleeping) continue;
            EntityID entity = rbArray->getEntity(i);
            if (!manager.hasComponent<EC_DOD_Spatial>(entity)) continue;
            auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);
            totalEnergy += 0.5f * rb.mass * glm::dot(spatial.velocity, spatial.velocity);
            totalEnergy += 0.5f * glm::dot(spatial.angVelocity, spatial.angVelocity);
            totalEnergy += rb.mass * 9.8f * spatial.position.y;
        }
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "[ENERGY] tick=" + std::to_string(s_EnergyTick) + " total=" + std::to_string(totalEnergy),
            LOGGING::LogLevel::INFORMATION);
    }
}
