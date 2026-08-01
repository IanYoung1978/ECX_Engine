#include "EC_PhysicsResolution.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kEpsilon = 1e-6f;
    constexpr float kPenetrationSlop = 0.01f;
    // Below this closing speed, treat the impact as inelastic (0 restitution)
    // regardless of the bodies' material restitution. Without this, a resting
    // object gets a fractional bounce-back every single frame from the tiny
    // closing velocity gravity itself adds before collision resolution runs -
    // perpetual micro-bounce that either jitters in place or, if the bounce
    // undershoots what gravity re-adds next frame, nets out as slow sinking.
    // Standard technique (Box2D calls this b2_velocityThreshold). Set high
    // enough to also cover a tumbling box's corner/edge striking the floor
    // mid-settle - that should read as an inelastic "thud" that kills the
    // tumble, not a bounce. Measured empirically: a cube ejected from a
    // collapsing stack can spin at 4-5 rad/s, so a corner (roughly half the
    // face diagonal from center, ~0.7-1.0 units) strikes the floor at
    // 3-5 m/s purely from rotation, on top of whatever it's translating at -
    // a 2.0 threshold let every one of those corner strikes bounce at 0.3
    // restitution instead of going inelastic, which sustains the tumble
    // indefinitely instead of killing it. Genuinely hard impacts (the ball
    // hitting the stack, ~5+ m/s of real linear closing speed) are still
    // well above this and bounce normally.
    constexpr float kRestitutionVelocityThreshold = 6.0f;
    // Fraction of excess penetration (beyond slop) that correctPosition
    // removes immediately, once per pair per tick. 1.0 would fully close
    // the gap in one shot but tends to overshoot/pop when several pairs on
    // the same body disagree about direction; 0.2-0.8 is the standard
    // range, correcting fully within a handful of ticks without visible
    // popping.
    constexpr float kPositionCorrectionPercent = 0.4f;

    struct ResolvedBody
    {
        float invMass = 0.0f;
        float restitution = 0.3f;
        float friction = 0.5f;
        float staticFriction = 0.5f;
        float rollingFriction = 0.05f;
        bool isStatic = true;
        glm::mat3 invInertiaWorld{ 0.0f };
    };

    ResolvedBody resolveBody(EC_DOD_EntityManager& manager, EntityID entity)
    {
        ResolvedBody body;
        if (manager.hasComponent<EC_DOD_RigidBody>(entity)) {
            const auto& rb = manager.getComponent<EC_DOD_RigidBody>(entity);
            body.isStatic = rb.isStatic;
            body.restitution = rb.restitution;
            body.friction = rb.friction;
            body.staticFriction = rb.staticFriction;
            body.rollingFriction = rb.rollingFriction;
            body.invMass = (rb.isStatic || rb.mass <= kEpsilon) ? 0.0f : (1.0f / rb.mass);
            if (body.invMass > 0.0f) {
                body.invInertiaWorld = EC_PhysicsResolution::computeInvInertiaWorld(entity, rb.mass);
            }
        }
        return body;
    }

    // Adds a linear+angular momentum contribution to entity's accumulator, if
    // it has one (entities without EC_DOD_RigidBody, e.g. static scene
    // geometry, silently receive nothing - correct, they never move).
    void addMomentum(EC_DOD_EntityManager& manager, EntityID entity,
        const glm::vec3& linearDelta, const glm::vec3& angularDelta)
    {
        if (!manager.hasComponent<EC_DOD_ImpulseAccumulator>(entity)) return;
        auto& accum = manager.getComponent<EC_DOD_ImpulseAccumulator>(entity);
        accum.deltaLinearMomentum += linearDelta;
        accum.deltaAngularMomentum += angularDelta;
    }

    // A body's velocity/angular velocity as resolution works through one
    // pair's manifold this tick - starts as a copy of the real
    // EC_DOD_Spatial values and is updated after every contact point, but
    // never written back to EC_DOD_Spatial itself (that still only happens
    // once, in EC_PhysicsSystem's apply step).
    struct LocalVelocity
    {
        glm::vec3 velocity;
        glm::vec3 angVelocity;
    };

    glm::vec3 velocityAtPoint(const LocalVelocity& local, const glm::vec3& r)
    {
        return local.velocity + glm::cross(local.angVelocity, r);
    }

    // Applies impulse to both bodies' running local velocities (so the next
    // contact point in this same manifold sees the effect) and, in
    // parallel, adds the same contribution to each body's real
    // EC_DOD_ImpulseAccumulator for EC_PhysicsSystem to apply once later.
    void applyImpulse(EC_DOD_EntityManager& manager, EntityID a, EntityID b,
        const ResolvedBody& bodyA, const ResolvedBody& bodyB,
        const glm::vec3& rA, const glm::vec3& rB, const glm::vec3& impulse,
        LocalVelocity& localA, LocalVelocity& localB)
    {
        const glm::vec3 angularA = -glm::cross(rA, impulse);
        const glm::vec3 angularB = glm::cross(rB, impulse);

        addMomentum(manager, a, -impulse, angularA);
        addMomentum(manager, b, impulse, angularB);

        localA.velocity += -impulse * bodyA.invMass;
        localA.angVelocity += bodyA.invInertiaWorld * angularA;
        localB.velocity += impulse * bodyB.invMass;
        localB.angVelocity += bodyB.invInertiaWorld * angularB;
    }

    // Computes the normal + friction impulse at one contact point, using
    // the standard accumulated-impulse (warm-started) pattern: cache.
    // normalImpulse already holds the running total from every earlier
    // iteration this tick, PLUS whatever carried over from last tick (see
    // accumulateImpulses) - this computes the INCREMENTAL correction still
    // needed given the current (already-updated) local velocity, adds it
    // to that running total, clamps the TOTAL (not just the increment) to
    // stay non-negative, and applies only the resulting delta. That's what
    // makes a persistent contact (a resting or tumbling box) converge
    // smoothly tick-to-tick instead of re-deriving - and re-injecting
    // solver noise from - a fresh answer every single frame.
    //
    // Contact points within the same manifold are resolved sequentially
    // (Gauss-Seidel) against localA/localB, which the caller updates in
    // place after every point - a resting/toppled box against a flat
    // surface has up to 4 points (see AABBVsAABB/OBBVsOBB), and solving
    // them independently would inject spurious net torque instead of
    // cancelling it. This is still entirely local to this one pair/
    // manifold - it never touches the real EC_DOD_Spatial, and cross-pair
    // ordering stays whatever order EC_PhysicsSystem's step 1 happens to
    // iterate pairs in.
    void accumulateContactPoint(EC_DOD_EntityManager& manager, EntityID a, EntityID b,
        const glm::vec3& posA, const glm::vec3& posB,
        LocalVelocity& localA, LocalVelocity& localB,
        const ResolvedBody& bodyA, const ResolvedBody& bodyB,
        const glm::vec3& contactPoint, const glm::vec3& normal, float invMassSum,
        EC_ContactImpulseCache& cache)
    {
        const glm::vec3 rA = contactPoint - posA;
        const glm::vec3 rB = contactPoint - posB;

        // Penetration depth is resolved entirely by correctPosition (a
        // direct, unbounded position correction run once per pair after
        // velocity resolution) - NOT by a velocity-space bias here.
        // Combining a Baumgarte bias with a direct position correction
        // double-corrects: the position fix removes the overlap, but the
        // bias-driven separating velocity it also added is still there and
        // keeps pushing the body apart on top of that, which reads as
        // bouncing/skittering. This impulse now only ever cancels real
        // closing velocity (with restitution), same as it would for a
        // pair that isn't penetrating at all.
        const glm::vec3 relativeVelocity = velocityAtPoint(localB, rB) - velocityAtPoint(localA, rA);
        const float velAlongNormal = glm::dot(relativeVelocity, normal);

        const glm::vec3 raCrossN = glm::cross(rA, normal);
        const glm::vec3 rbCrossN = glm::cross(rB, normal);
        const float angularTermA = glm::dot(glm::cross(bodyA.invInertiaWorld * raCrossN, rA), normal);
        const float angularTermB = glm::dot(glm::cross(bodyB.invInertiaWorld * rbCrossN, rB), normal);
        const float denom = invMassSum + angularTermA + angularTermB;
        if (denom <= kEpsilon) {
            return;
        }

        // --- Normal impulse: accumulated/clamped pattern, seeded from
        // cache.normalImpulse (warm start). ---
        const float restitution = (-velAlongNormal > kRestitutionVelocityThreshold)
            ? std::min(bodyA.restitution, bodyB.restitution)
            : 0.0f;
        const float lambda = -(1.0f + restitution) * velAlongNormal / denom;
        const float newNormalTotal = std::max(0.0f, cache.normalImpulse + lambda);
        const float deltaNormal = newNormalTotal - cache.normalImpulse;
        cache.normalImpulse = newNormalTotal;

        if (std::abs(deltaNormal) > kEpsilon) {
            applyImpulse(manager, a, b, bodyA, bodyB, rA, rB, deltaNormal * normal, localA, localB);
        }

        if (newNormalTotal <= 0.0f) {
            return; // nothing to grip for friction either
        }

        // --- Rolling resistance: a small torque at this contact point
        // opposing RELATIVE ANGULAR velocity (rolling), independent of
        // whether there's any tangential SLIP here at all - a pure rolling
        // contact (a box balanced on an edge, a ball on the floor) has
        // ZERO relative tangential velocity by definition, which is
        // exactly the case ordinary sliding/static friction below can
        // never resist (that's the whole point of rollingFriction - see
        // EC_DOD_RigidBody). Computed BEFORE the tangential-slip early
        // return above would otherwise skip it entirely. Applied as a
        // pure angular impulse - no lever arm, no linear reaction -
        // since rolling resistance is a torque effect, not a force at a
        // point; not warm-started across ticks, same as tangential
        // friction (recomputed fresh from current velocity each call).
        {
            const glm::vec3 relativeAngVel = localB.angVelocity - localA.angVelocity;
            const float relativeAngSpeed2 = glm::dot(relativeAngVel, relativeAngVel);
            if (relativeAngSpeed2 > kEpsilon) {
                const float relativeAngSpeed = std::sqrt(relativeAngSpeed2);
                const glm::vec3 rollAxis = relativeAngVel / relativeAngSpeed;
                const float rollingCoefficient = std::sqrt(bodyA.rollingFriction * bodyB.rollingFriction);
                const float maxRollingImpulse = rollingCoefficient * newNormalTotal;

                const float angularDenomA = glm::dot(bodyA.invInertiaWorld * rollAxis, rollAxis);
                const float angularDenomB = glm::dot(bodyB.invInertiaWorld * rollAxis, rollAxis);
                const float rollDenom = angularDenomA + angularDenomB;
                if (rollDenom > kEpsilon) {
                    const float fullStopImpulse = relativeAngSpeed / rollDenom;
                    const float rollingImpulseMag = std::min(fullStopImpulse, maxRollingImpulse);
                    const glm::vec3 rollingAngularImpulse = -rollingImpulseMag * rollAxis;

                    localA.angVelocity -= bodyA.invInertiaWorld * rollingAngularImpulse;
                    localB.angVelocity += bodyB.invInertiaWorld * rollingAngularImpulse;
                    addMomentum(manager, a, glm::vec3(0.0f), -rollingAngularImpulse);
                    addMomentum(manager, b, glm::vec3(0.0f), rollingAngularImpulse);
                }
            }
        }

        // --- Friction: Coulomb friction clamped to this point's total
        // normal impulse - re-reads relative velocity now that the normal
        // impulse above has already updated localA/localB. Not
        // warm-started across ticks (see header comment) - always starts
        // this tick's accumulation from zero.
        const glm::vec3 relativeVelocityAfterNormal = velocityAtPoint(localB, rB) - velocityAtPoint(localA, rA);
        glm::vec3 tangent = relativeVelocityAfterNormal - normal * glm::dot(relativeVelocityAfterNormal, normal);
        const float tangentLen2 = glm::dot(tangent, tangent);
        if (tangentLen2 <= kEpsilon) {
            return;
        }
        tangent /= std::sqrt(tangentLen2);

        const glm::vec3 raCrossT = glm::cross(rA, tangent);
        const glm::vec3 rbCrossT = glm::cross(rB, tangent);
        const float angularTermTA = glm::dot(glm::cross(bodyA.invInertiaWorld * raCrossT, rA), tangent);
        const float angularTermTB = glm::dot(glm::cross(bodyB.invInertiaWorld * rbCrossT, rB), tangent);
        const float denomT = invMassSum + angularTermTA + angularTermTB;
        if (denomT <= kEpsilon) {
            return;
        }

        // lambdaT is the impulse that would fully cancel tangential
        // velocity this sub-step - i.e. "stick" the contact completely.
        // Static vs kinetic Coulomb friction: if that stick impulse (added
        // to whatever's already accumulated) fits within the STATIC cone,
        // apply it in full - the contact holds, zero slip results. Only
        // once it exceeds that (real, usually higher) threshold does it
        // clamp down to the KINETIC cone instead, leaving the contact
        // genuinely sliding under a capped friction force. Using a single
        // coefficient for both would make a resting stack just as easy to
        // start sliding as to keep sliding, understating how much a real
        // contact resists breaking static grip in the first place.
        const float lambdaT = -glm::dot(relativeVelocityAfterNormal, tangent) / denomT;
        const float staticCoefficient = std::sqrt(bodyA.staticFriction * bodyB.staticFriction);
        const float kineticCoefficient = std::sqrt(bodyA.friction * bodyB.friction);
        const float maxStaticFriction = staticCoefficient * newNormalTotal;
        const float maxKineticFriction = kineticCoefficient * newNormalTotal;
        const float unclampedTangentTotal = cache.tangentImpulse + lambdaT;
        const float newTangentTotal = (std::abs(unclampedTangentTotal) <= maxStaticFriction)
            ? unclampedTangentTotal
            : std::clamp(unclampedTangentTotal, -maxKineticFriction, maxKineticFriction);
        const float deltaTangent = newTangentTotal - cache.tangentImpulse;
        cache.tangentImpulse = newTangentTotal;

        if (std::abs(deltaTangent) > kEpsilon) {
            applyImpulse(manager, a, b, bodyA, bodyB, rA, rB, deltaTangent * tangent, localA, localB);
        }
    }
}

namespace EC_PhysicsResolution
{
    glm::mat3 computeInvInertiaWorld(EntityID entity, float mass)
    {
        auto& manager = EC_DOD_EntityManager::getInstance();
        if (!manager.hasComponent<EC_DOD_Collider>(entity)) return glm::mat3(0.0f);
        const auto& collider = manager.getComponent<EC_DOD_Collider>(entity);

        glm::vec3 invInertiaDiag(0.0f);
        glm::mat3 orientation(1.0f);

        if (collider.type == EC_DOD_Collider::Type::Sphere)
        {
            const float r = collider.radius;
            const float I = (2.0f / 5.0f) * mass * r * r;
            invInertiaDiag = glm::vec3(I > kEpsilon ? 1.0f / I : 0.0f);
        }
        else if (collider.type == EC_DOD_Collider::Type::AABB || collider.type == EC_DOD_Collider::Type::OBB)
        {
            const glm::vec3 full = collider.extents * 2.0f;
            const glm::vec3 I(
                (1.0f / 12.0f) * mass * (full.y * full.y + full.z * full.z),
                (1.0f / 12.0f) * mass * (full.x * full.x + full.z * full.z),
                (1.0f / 12.0f) * mass * (full.x * full.x + full.y * full.y));
            invInertiaDiag = glm::vec3(
                I.x > kEpsilon ? 1.0f / I.x : 0.0f,
                I.y > kEpsilon ? 1.0f / I.y : 0.0f,
                I.z > kEpsilon ? 1.0f / I.z : 0.0f);

            if (collider.type == EC_DOD_Collider::Type::OBB && manager.hasComponent<EC_DOD_Spatial>(entity))
            {
                const auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);
                orientation = glm::mat3(glm::normalize(spatial.right), glm::normalize(spatial.up), glm::normalize(spatial.direction));
            }
        }
        else
        {
            return glm::mat3(0.0f);
        }

        glm::mat3 invInertiaLocal(0.0f);
        invInertiaLocal[0][0] = invInertiaDiag.x;
        invInertiaLocal[1][1] = invInertiaDiag.y;
        invInertiaLocal[2][2] = invInertiaDiag.z;

        return orientation * invInertiaLocal * glm::transpose(orientation);
    }

    void warmStartPair(EntityID a, EntityID b, const CollisionManifold& manifold,
        std::vector<EC_ContactImpulseCache>& contactCache)
    {
        if (glm::dot(manifold.contactNormal, manifold.contactNormal) < kEpsilon) { contactCache.clear(); return; }
        if (manifold.contactPoints.empty()) { contactCache.clear(); return; }

        auto& manager = EC_DOD_EntityManager::getInstance();
        if (!manager.hasComponent<EC_DOD_Spatial>(a) || !manager.hasComponent<EC_DOD_Spatial>(b)) {
            contactCache.clear();
            return;
        }
        const auto& spatialA = manager.getComponent<EC_DOD_Spatial>(a);
        const auto& spatialB = manager.getComponent<EC_DOD_Spatial>(b);

        // Match this tick's contact points to last tick's cache by closest
        // position (within a small radius) so each point's accumulated
        // normal impulse carries over. Unmatched points (a genuinely new
        // contact) start at zero, same as with no warm starting at all.
        //
        // One-to-one: each OLD cache entry can be claimed by at most one
        // NEW point (usedOld tracks that). Without this, a manifold that
        // grows point count tick-to-tick (e.g. a box settling face-flat,
        // 1 point -> 2 -> 4 as it rocks down) can have several new points
        // all fall within the matching radius of the SAME old point, each
        // independently inheriting its full cached impulse - duplicating a
        // single real impulse 2x/3x/4x and injecting real, non-physical
        // energy into the system on exactly the settling transitions this
        // was meant to smooth out.
        std::vector<EC_ContactImpulseCache> newCache(manifold.contactPoints.size());
        std::vector<bool> usedOld(contactCache.size(), false);
        for (size_t i = 0; i < manifold.contactPoints.size(); i++) {
            newCache[i].point = manifold.contactPoints[i];
            float bestDist2 = 0.25f; // 0.5 unit matching radius, squared
            int bestMatch = -1;
            for (size_t j = 0; j < contactCache.size(); j++) {
                if (usedOld[j]) continue;
                const glm::vec3 diff = manifold.contactPoints[i] - contactCache[j].point;
                const float d2 = glm::dot(diff, diff);
                if (d2 < bestDist2) { bestDist2 = d2; bestMatch = static_cast<int>(j); }
            }
            if (bestMatch >= 0) {
                newCache[i].normalImpulse = contactCache[bestMatch].normalImpulse;
                usedOld[bestMatch] = true;
            }
        }

        // Apply every matched point's carried-over normal impulse directly
        // to the real EC_DOD_ImpulseAccumulator, once, so every
        // previewVelocity call this tick (across every later solver pass)
        // reflects it as this tick's starting guess.
        for (size_t i = 0; i < newCache.size(); i++) {
            if (newCache[i].normalImpulse <= kEpsilon) continue;
            const glm::vec3 rA = manifold.contactPoints[i] - spatialA.position;
            const glm::vec3 rB = manifold.contactPoints[i] - spatialB.position;
            const glm::vec3 impulse = newCache[i].normalImpulse * manifold.contactNormal;
            addMomentum(manager, a, -impulse, -glm::cross(rA, impulse));
            addMomentum(manager, b, impulse, glm::cross(rB, impulse));
        }

        contactCache = std::move(newCache);
    }

    void accumulateImpulses(EntityID a, EntityID b, const CollisionManifold& manifold,
        const glm::vec3& velA, const glm::vec3& angVelA,
        const glm::vec3& velB, const glm::vec3& angVelB,
        std::vector<EC_ContactImpulseCache>& contactCache)
    {
        if (glm::dot(manifold.contactNormal, manifold.contactNormal) < kEpsilon) return;
        if (manifold.contactPoints.empty()) return;
        if (contactCache.size() != manifold.contactPoints.size()) return; // warmStartPair wasn't called for this tick's manifold

        auto& manager = EC_DOD_EntityManager::getInstance();
        ResolvedBody bodyA = resolveBody(manager, a);
        ResolvedBody bodyB = resolveBody(manager, b);

        const float invMassSum = bodyA.invMass + bodyB.invMass;
        if (invMassSum <= kEpsilon) return;

        if (!manager.hasComponent<EC_DOD_Spatial>(a) || !manager.hasComponent<EC_DOD_Spatial>(b)) return;

        const auto& spatialA = manager.getComponent<EC_DOD_Spatial>(a);
        const auto& spatialB = manager.getComponent<EC_DOD_Spatial>(b);

        LocalVelocity localA{ velA, angVelA };
        LocalVelocity localB{ velB, angVelB };

        // Revisit this pair's own manifold several times per call (standard
        // sequential-impulse practice - Box2D/Bullet default to ~4-8), not
        // just once. A single pass measured empirically as insufficient:
        // at a single grazing corner mid-topple, the normal impulse from
        // one pass often doesn't yet reflect the body's full weight, so
        // friction's budget (mu * that impulse) comes up short and real
        // slip persists that a second/third pass - seeing the already-
        // improved local velocity - converges away.
        constexpr int kManifoldIterations = 4;
        for (int iteration = 0; iteration < kManifoldIterations; iteration++) {
            for (size_t i = 0; i < manifold.contactPoints.size(); i++) {
                accumulateContactPoint(manager, a, b, spatialA.position, spatialB.position,
                    localA, localB, bodyA, bodyB,
                    manifold.contactPoints[i], manifold.contactNormal, invMassSum,
                    contactCache[i]);
            }
        }
    }

    bool shouldResolve(EntityID a, EntityID b)
    {
        auto& manager = EC_DOD_EntityManager::getInstance();
        const bool aHasRB = manager.hasComponent<EC_DOD_RigidBody>(a);
        const bool bHasRB = manager.hasComponent<EC_DOD_RigidBody>(b);

        // Neither side is a dynamic body (e.g. two purely static/visual
        // entities sharing a collider) - nothing for physics to do.
        if (!aHasRB && !bHasRB) return false;

        // A missing RigidBody, or an explicit isStatic, behaves like "asleep"
        // for this purpose - it never has real motion and never needs waking.
        const bool aAsleep = aHasRB && manager.getComponent<EC_DOD_RigidBody>(a).isSleeping;
        const bool bAsleep = bHasRB && manager.getComponent<EC_DOD_RigidBody>(b).isSleeping;
        const bool aAtRest = !aHasRB || aAsleep || manager.getComponent<EC_DOD_RigidBody>(a).isStatic;
        const bool bAtRest = !bHasRB || bAsleep || manager.getComponent<EC_DOD_RigidBody>(b).isStatic;

        if (aAtRest && bAtRest) {
            return false; // both sides at rest relative to each other - resolving
                           // this every frame is exactly what re-injects drift.
        }

        // The other side has real motion - wake whichever side is asleep so
        // it actually reacts this frame.
        if (aAsleep) {
            auto& rb = manager.getComponent<EC_DOD_RigidBody>(a);
            rb.isSleeping = false;
            rb.sleepTimer = 0.0f;
        }
        if (bAsleep) {
            auto& rb = manager.getComponent<EC_DOD_RigidBody>(b);
            rb.isSleeping = false;
            rb.sleepTimer = 0.0f;
        }

        return true;
    }

    void correctPosition(EntityID a, EntityID b, const CollisionManifold& manifold)
    {
        if (glm::dot(manifold.contactNormal, manifold.contactNormal) < kEpsilon) return;
        if (manifold.penetrationDepth <= kPenetrationSlop) return;
        if (manifold.contactPoints.empty()) return;

        auto& manager = EC_DOD_EntityManager::getInstance();
        ResolvedBody bodyA = resolveBody(manager, a);
        ResolvedBody bodyB = resolveBody(manager, b);

        const float invMassSum = bodyA.invMass + bodyB.invMass;
        if (invMassSum <= kEpsilon) return;

        if (!manager.hasComponent<EC_DOD_Spatial>(a) || !manager.hasComponent<EC_DOD_Spatial>(b)) return;
        auto& spatialA = manager.getComponent<EC_DOD_Spatial>(a);
        auto& spatialB = manager.getComponent<EC_DOD_Spatial>(b);

        // Translation only - deliberately does NOT touch orientation.
        // correctPosition's job is cleaning up small residual overlap left
        // over after velocity resolution, not driving rotation - rotation
        // belongs entirely to the velocity solver (accumulateImpulses),
        // where it's expressed as real angVelocity: integrated normally,
        // damped normally, and visible to the sleep system. An earlier
        // version of this function also nudged orientation directly here
        // (a per-point pseudo-torque, mirroring the velocity solver's
        // r-cross-impulse formula) - for a body sitting in a SUSTAINED
        // asymmetric contact (e.g. leaning against a neighbour rather than
        // resting flat), gravity regenerates the same small overlap every
        // tick, so that correction reapplied every tick too, in the same
        // direction, accumulating into a large rotation over many seconds -
        // entirely outside spatial.angVelocity, so the sleep system (which
        // only ever looks at velocity) had no way to see or stop it. Straight
        // translation split evenly across every point avoids that failure
        // mode entirely: it can only ever separate the bodies, never spin
        // one - if something in the scene needs a body to rotate, that
        // has to come from a real, measurable angular velocity, not a
        // position-space nudge with no velocity behind it.
        const float excessDepth = manifold.penetrationDepth - kPenetrationSlop;
        const float correctionMag = excessDepth / invMassSum * kPositionCorrectionPercent;
        const glm::vec3 correction = correctionMag * manifold.contactNormal;

        if (bodyA.invMass > 0.0f) spatialA.position -= correction * bodyA.invMass;
        if (bodyB.invMass > 0.0f) spatialB.position += correction * bodyB.invMass;
    }

    void wakeIfSleeping(EntityID entity)
    {
        auto& manager = EC_DOD_EntityManager::getInstance();
        if (!manager.hasComponent<EC_DOD_RigidBody>(entity)) return;

        auto& rb = manager.getComponent<EC_DOD_RigidBody>(entity);
        if (rb.isSleeping) {
            rb.isSleeping = false;
            rb.sleepTimer = 0.0f;
        }
    }

    void previewVelocity(EntityID entity, glm::vec3& outVel, glm::vec3& outAngVel)
    {
        auto& manager = EC_DOD_EntityManager::getInstance();
        outVel = glm::vec3(0.0f);
        outAngVel = glm::vec3(0.0f);
        if (manager.hasComponent<EC_DOD_Spatial>(entity)) {
            const auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);
            outVel = spatial.velocity;
            outAngVel = spatial.angVelocity;
        }

        if (!manager.hasComponent<EC_DOD_RigidBody>(entity)) return;
        const auto& rb = manager.getComponent<EC_DOD_RigidBody>(entity);
        if (rb.isStatic || rb.mass <= kEpsilon) return;
        if (!manager.hasComponent<EC_DOD_ImpulseAccumulator>(entity)) return;

        const auto& accum = manager.getComponent<EC_DOD_ImpulseAccumulator>(entity);
        const float invMass = 1.0f / rb.mass;
        const glm::mat3 invInertiaWorld = computeInvInertiaWorld(entity, rb.mass);
        outVel += accum.deltaLinearMomentum * invMass;
        outAngVel += invInertiaWorld * accum.deltaAngularMomentum;
    }
}
