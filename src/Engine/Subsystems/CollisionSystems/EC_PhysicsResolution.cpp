#include "EC_PhysicsResolution.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kEpsilon = 1e-6f;
    // Below this closing speed a contact is resting (its velocity is just gravity's per-substep
    // residual) and restitution is treated as zero; otherwise (1+restitution) amplifies the residual into
    // a phantom bounce and jitter. Set above a typical per-substep residual (~0.04 m/s), well below a real
    // impact.
    constexpr float kRestingVelocityThreshold = 0.2f;

    // Bullet's btPlaneSpace1: a stable orthonormal basis {t1, t2} perpendicular to n, from n alone. A
    // tangent derived from relative velocity is unstable near rest, where normalizing a tiny noisy
    // vector makes the friction direction swing.
    void planeSpace(const glm::vec3& n, glm::vec3& t1, glm::vec3& t2)
    {
        if (std::abs(n.z) > 0.7071067f) {
            const float a = n.y * n.y + n.z * n.z;
            const float k = 1.0f / std::sqrt(a);
            t1 = glm::vec3(0.0f, -n.z * k, n.y * k);
        } else {
            const float a = n.x * n.x + n.y * n.y;
            const float k = 1.0f / std::sqrt(a);
            t1 = glm::vec3(-n.y * k, n.x * k, 0.0f);
        }
        t2 = glm::cross(n, t1);
    }
}

namespace EC_PhysicsResolution
{
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

    BodyState recordBody(EntityID entity)
    {
        BodyState body;
        auto& manager = EC_DOD_EntityManager::getInstance();

        if (manager.hasComponent<EC_DOD_Spatial>(entity)) {
            const auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);
            body.recordedVelocity = spatial.velocity;
            body.recordedAngVelocity = spatial.angVelocity;
        }

        if (manager.hasComponent<EC_DOD_RigidBody>(entity)) {
            const auto& rb = manager.getComponent<EC_DOD_RigidBody>(entity);
            // Static and Kinematic both get treated as infinite mass here (invMass 0) -
            // neither should ever be pushed by collision response, only Dynamic bodies are
            // actually resolved by the impulse solver (see EC_BodyType's own comment).
            if (rb.bodyType == EC_BodyType::Dynamic && rb.mass > kEpsilon) {
                body.invMass = 1.0f / rb.mass;
                body.invInertiaWorld = computeInvInertiaWorld(entity, rb.mass);
            }
            body.restitution = rb.restitution;
            body.staticFriction = rb.staticFriction;
            body.kineticFriction = rb.friction;
        }

        return body;
    }

    glm::vec3 estimatedVelocityAtPoint(const BodyState& body, const glm::vec3& r)
    {
        const glm::vec3 velocity = body.recordedVelocity + body.accumulatedLinearImpulse * body.invMass;
        const glm::vec3 angVelocity = body.recordedAngVelocity + body.invInertiaWorld * body.accumulatedAngularImpulse;
        return velocity + glm::cross(angVelocity, r);
    }

    void applyWarmStart(
        BodyState& bodyA, BodyState& bodyB,
        const glm::vec3& posA, const glm::vec3& posB,
        const glm::vec3& contactPoint, const glm::vec3& normal,
        const EC_ContactImpulseCache& cache)
    {
        const glm::vec3 rA = contactPoint - posA;
        const glm::vec3 rB = contactPoint - posB;

        glm::vec3 t1, t2;
        planeSpace(normal, t1, t2);
        const glm::vec3 impulse = cache.normalImpulse * normal + cache.tangentImpulse.x * t1 + cache.tangentImpulse.y * t2;
        if (glm::dot(impulse, impulse) <= kEpsilon * kEpsilon) return;

        bodyA.accumulatedLinearImpulse -= impulse;
        bodyA.accumulatedAngularImpulse -= glm::cross(rA, impulse);
        bodyB.accumulatedLinearImpulse += impulse;
        bodyB.accumulatedAngularImpulse += glm::cross(rB, impulse);
    }

    void accumulateContactImpulse(
        BodyState& bodyA, BodyState& bodyB,
        const glm::vec3& posA, const glm::vec3& posB,
        const glm::vec3& contactPoint, const glm::vec3& normal,
        EC_ContactImpulseCache& cache)
    {
        const glm::vec3 rA = contactPoint - posA;
        const glm::vec3 rB = contactPoint - posB;

        const float invMassSum = bodyA.invMass + bodyB.invMass;
        if (invMassSum <= kEpsilon) return;

        glm::vec3 t1, t2;
        planeSpace(normal, t1, t2);
        const glm::vec3 axes[3] = { normal, t1, t2 };

        glm::vec3 raCross[3], rbCross[3];
        for (int i = 0; i < 3; i++) {
            raCross[i] = glm::cross(rA, axes[i]);
            rbCross[i] = glm::cross(rB, axes[i]);
        }

        // Effective-mass matrix for the 3-axis contact space (normal + 2 tangents), coupled through the
        // bodies' rotational inertia rather than solved as separate 1D problems.
        glm::mat3 K;
        for (int col = 0; col < 3; col++) {
            for (int row = 0; row < 3; row++) {
                K[col][row] = (row == col ? invMassSum : 0.0f)
                    + glm::dot(raCross[row], bodyA.invInertiaWorld * raCross[col])
                    + glm::dot(rbCross[row], bodyB.invInertiaWorld * rbCross[col]);
            }
        }

        const glm::vec3 relativeVelocity = estimatedVelocityAtPoint(bodyB, rB) - estimatedVelocityAtPoint(bodyA, rA);
        const float velAlongNormal = glm::dot(relativeVelocity, normal);
        const float velAlongT1 = glm::dot(relativeVelocity, t1);
        const float velAlongT2 = glm::dot(relativeVelocity, t2);

        const float restitution = (std::abs(velAlongNormal) < kRestingVelocityThreshold)
            ? 0.0f
            : std::min(bodyA.restitution, bodyB.restitution);
        const glm::vec3 targetDeltaVel(-(1.0f + restitution) * velAlongNormal, -velAlongT1, -velAlongT2);

        // A near-singular K blows up under direct inversion, so fall back to the decoupled (off-diagonals
        // zeroed) solve: exact when well-conditioned, merely uncoupled (never unstable) otherwise.
        glm::vec3 lambda;
        const float det = glm::determinant(K);
        if (std::abs(det) <= 0.01f * K[0][0] * K[1][1] * K[2][2]) {
            lambda = glm::vec3(
                K[0][0] > kEpsilon ? targetDeltaVel.x / K[0][0] : 0.0f,
                K[1][1] > kEpsilon ? targetDeltaVel.y / K[1][1] : 0.0f,
                K[2][2] > kEpsilon ? targetDeltaVel.z / K[2][2] : 0.0f);
        } else {
            lambda = glm::inverse(K) * targetDeltaVel;
        }

        float newNormalTotal = std::max(0.0f, cache.normalImpulse + lambda.x);

        // Coulomb friction disc (not two independent 1D clamps): the
        // tangent impulse is a single 2D vector clamped by its magnitude,
        // matching the physical friction cone regardless of slide direction.
        const float staticCoefficient = std::sqrt(bodyA.staticFriction * bodyB.staticFriction);
        const float kineticCoefficient = std::sqrt(bodyA.kineticFriction * bodyB.kineticFriction);
        const float maxStaticFriction = staticCoefficient * newNormalTotal;
        const glm::vec2 unclampedTangentTotal = cache.tangentImpulse + glm::vec2(lambda.y, lambda.z);
        const float tangentMag = glm::length(unclampedTangentTotal);

        glm::vec2 newTangentTotal;
        if (tangentMag <= maxStaticFriction || tangentMag <= kEpsilon) {
            newTangentTotal = unclampedTangentTotal;
        } else {
            // Friction cone exceeded: solve the normal and tangent totals together (Millington). With
            // tangentTotal fixed at kineticCoefficient*Pn*direction, the normal-row equation
            // K00*(Pn-cache.n) + K10*(tangentTotal.x-cache.t.x) + K20*(tangentTotal.y-cache.t.y) = targetDeltaVel.x
            // gives Pn directly:
            const glm::vec2 direction = unclampedTangentTotal / tangentMag;
            const float coupling = K[1][0] * direction.x + K[2][0] * direction.y;
            const float denom2 = K[0][0] + kineticCoefficient * coupling;
            if (std::abs(denom2) > kEpsilon) {
                const float pn = (targetDeltaVel.x + K[0][0] * cache.normalImpulse
                    + K[1][0] * cache.tangentImpulse.x + K[2][0] * cache.tangentImpulse.y) / denom2;
                newNormalTotal = std::max(0.0f, pn);
            }
            newTangentTotal = direction * (kineticCoefficient * newNormalTotal);
        }
        const float deltaNormal = newNormalTotal - cache.normalImpulse;
        cache.normalImpulse = newNormalTotal;
        const glm::vec2 deltaTangent = newTangentTotal - cache.tangentImpulse;
        cache.tangentImpulse = newTangentTotal;

        const glm::vec3 impulse = deltaNormal * normal + deltaTangent.x * t1 + deltaTangent.y * t2;
        if (glm::dot(impulse, impulse) > kEpsilon * kEpsilon) {
            bodyA.accumulatedLinearImpulse -= impulse;
            bodyA.accumulatedAngularImpulse -= glm::cross(rA, impulse);
            bodyB.accumulatedLinearImpulse += impulse;
            bodyB.accumulatedAngularImpulse += glm::cross(rB, impulse);
        }
    }
}
