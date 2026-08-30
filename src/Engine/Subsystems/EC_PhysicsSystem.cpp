#include "Engine/Subsystems/EC_PhysicsSystem.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Entity/EC_DOD_Types.h"
#include "Components/EC_DOD_Components.h"
#include "Engine/Subsystems/CollisionSystems/EC_PhysicsResolution.h"
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"
#include "Engine/Subsystems/CollisionSystems/EC_CollisionShapes.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <unordered_map>
#include <sstream>
#include "Logging/ECX_Logging.h"

EC_PhysicsSystem::EC_PhysicsSystem() {}
EC_PhysicsSystem::~EC_PhysicsSystem() {}
void EC_PhysicsSystem::init(ECXMessenger& messenger, EC_Game& game) {}

namespace {
    constexpr glm::vec3 kGravity(0.0f, -9.8f, 0.0f);
    // Momentum, not raw velocity - mass/inertia-scaled, so the same
    // physical threshold applies consistently regardless of a body's mass
    // or shape.
    constexpr float kSleepLinearMomentumThreshold = 0.05f;
    constexpr float kSleepAngularMomentumThreshold = 0.01f;
    constexpr float kTimeToSleep = 1.0f;
    constexpr int kSolverPasses = 4;
    // Roughly 10 log lines/sec at 4 substeps * ~60Hz tick.
    constexpr int kDebugLogInterval = 24;
    // Positional correction: linear projection + slop (Baumgarte-style),
    // kept fully separate from the velocity solve above so the two never
    // double-correct the same overlap.
    constexpr float kPenetrationSlop = 0.01f;
    constexpr float kPositionCorrectionPercent = 0.4f;
}

void EC_PhysicsSystem::update(const float& deltaTimeS, EC_Game& game) {
    auto& manager = EC_DOD_EntityManager::getInstance();

    // --- Gravity applied first, before the solve, so the constraint solve
    // below sees and cancels THIS tick's gravity directly. Applying it after
    // instead leaves the solve chasing last tick's residual one step behind
    // forever, so the cached normal impulse a resting contact converges to
    // never reflects the real steady-state weight - it keeps ratcheting up
    // by a fresh increment each tick, understating the friction budget
    // (proportional to that cached impulse) for as long as it takes to
    // build up. ---
    {
        auto* rbArrayGravity = manager.getComponentArray<EC_DOD_RigidBody>();
        if (rbArrayGravity) {
            std::shared_lock gravityLock(rbArrayGravity->getMutex());
            auto& rigidBodiesGravity = rbArrayGravity->getData();
            for (size_t i = 0; i < rigidBodiesGravity.size(); i++) {
                auto& rb = rigidBodiesGravity[i];
                if (rb.isStatic || rb.isSleeping) continue;
                EntityID entity = rbArrayGravity->getEntity(i);
                if (!manager.hasComponent<EC_DOD_Spatial>(entity)) continue;
                manager.getComponent<EC_DOD_Spatial>(entity).velocity += kGravity * deltaTimeS;
            }
        }
    }

    if (m_PairManager) {
        // --- RECORD: every body touched by at least one currently-
        // colliding pair gets its real velocity/mass/inertia read exactly
        // once, up front. This recorded state is never mutated again this
        // tick - everything below only ever adds to a separate accumulator. ---
        std::unordered_map<EntityID, EC_PhysicsResolution::BodyState> bodies;
        // Every colliding pair's contact points, per entity - published
        // below as EC_DOD_DebugContacts so debug rendering (F1) can draw
        // them via the normal shared_mutex-protected component-array path,
        // instead of reading EC_PairManager (physics-thread-internal) from
        // the render thread.
        std::unordered_map<EntityID, std::vector<glm::vec3>> bodyContactPoints;
        for (EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
            if (!pair.m_Colliding) continue;
            bodies.try_emplace(pair.body_A, EC_PhysicsResolution::recordBody(pair.body_A));
            bodies.try_emplace(pair.body_B, EC_PhysicsResolution::recordBody(pair.body_B));

            auto& pointsA = bodyContactPoints[pair.body_A];
            auto& pointsB = bodyContactPoints[pair.body_B];
            pointsA.insert(pointsA.end(), pair.m_CollisionPoints.begin(), pair.m_CollisionPoints.end());
            pointsB.insert(pointsB.end(), pair.m_CollisionPoints.begin(), pair.m_CollisionPoints.end());
        }

        // Only entities actually in bodyContactPoints get written; anything
        // that had contacts last tick but not this tick gets explicitly
        // cleared so stale markers don't linger.
        std::vector<EntityID> currentContactEntities;
        currentContactEntities.reserve(bodyContactPoints.size());
        for (const auto& [entity, points] : bodyContactPoints) {
            manager.addComponent(entity, EC_DOD_DebugContacts{ points });
            currentContactEntities.push_back(entity);
        }
        for (EntityID entity : m_LastDebugContactEntities) {
            if (bodyContactPoints.find(entity) == bodyContactPoints.end()) {
                manager.addComponent(entity, EC_DOD_DebugContacts{});
            }
        }
        m_LastDebugContactEntities = std::move(currentContactEntities);

        // --- WARM START: re-apply every contact point's previously
        // converged impulse in full, before any new pass runs. Kinetic
        // friction is an ongoing force - it needs to keep decelerating a
        // sliding body every substep, not just once - so the cached
        // impulse has to be reapplied fresh each tick, not merely used as
        // a baseline that only the CHANGE from is ever applied to real
        // velocity (see EC_PhysicsResolution::applyWarmStart). ---
        for (EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
            if (!pair.m_Colliding) continue;

            auto& bodyA = bodies.at(pair.body_A);
            auto& bodyB = bodies.at(pair.body_B);
            if (bodyA.invMass <= 0.0f && bodyB.invMass <= 0.0f) continue;

            if (pair.m_ContactCache.size() != pair.m_CollisionPoints.size()) {
                pair.m_ContactCache.assign(pair.m_CollisionPoints.size(), EC_ContactImpulseCache{});
            }

            glm::vec3 posA(0.0f), posB(0.0f);
            if (manager.hasComponent<EC_DOD_Spatial>(pair.body_A))
                posA = manager.getComponent<EC_DOD_Spatial>(pair.body_A).position;
            if (manager.hasComponent<EC_DOD_Spatial>(pair.body_B))
                posB = manager.getComponent<EC_DOD_Spatial>(pair.body_B).position;

            for (size_t i = 0; i < pair.m_CollisionPoints.size(); i++) {
                EC_PhysicsResolution::applyWarmStart(
                    bodyA, bodyB, posA, posB,
                    pair.m_CollisionPoints[i], pair.m_ContactNormal,
                    pair.m_ContactCache[i]);
            }
        }

        // --- ITERATE: every contact point on every colliding pair
        // contributes an incremental impulse each pass. Repeating the whole
        // pass over every pair (rather than just each pair's own manifold)
        // is what lets a multi-body system - a stack - converge within one
        // tick: resolving the bottom pair changes velocities the next pair
        // up needs to see. ---
        for (int pass = 0; pass < kSolverPasses; pass++) {
            for (EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
                if (!pair.m_Colliding) continue;

                auto& bodyA = bodies.at(pair.body_A);
                auto& bodyB = bodies.at(pair.body_B);
                if (bodyA.invMass <= 0.0f && bodyB.invMass <= 0.0f) continue;

                if (pair.m_ContactCache.size() != pair.m_CollisionPoints.size()) {
                    pair.m_ContactCache.assign(pair.m_CollisionPoints.size(), EC_ContactImpulseCache{});
                }

                glm::vec3 posA(0.0f), posB(0.0f);
                if (manager.hasComponent<EC_DOD_Spatial>(pair.body_A))
                    posA = manager.getComponent<EC_DOD_Spatial>(pair.body_A).position;
                if (manager.hasComponent<EC_DOD_Spatial>(pair.body_B))
                    posB = manager.getComponent<EC_DOD_Spatial>(pair.body_B).position;

                for (size_t i = 0; i < pair.m_CollisionPoints.size(); i++) {
                    pair.m_ContactCache[i].point = pair.m_CollisionPoints[i];
                    EC_PhysicsResolution::accumulateContactImpulse(
                        bodyA, bodyB, posA, posB,
                        pair.m_CollisionPoints[i], pair.m_ContactNormal,
                        pair.m_ContactCache[i]);

                    if (m_LogFriction && pass == kSolverPasses - 1 && ++m_ContactLogCounter >= kDebugLogInterval) {
                        m_ContactLogCounter = 0;
                        const auto& c = pair.m_ContactCache[i];
                        const float staticCoeff = std::sqrt(bodyA.staticFriction * bodyB.staticFriction);
                        const float kineticCoeff = std::sqrt(bodyA.kineticFriction * bodyB.kineticFriction);
                        std::ostringstream oss;
                        oss << "[FRICTION] A=" << pair.body_A << " B=" << pair.body_B
                            << " point=" << i
                            << " normalImpulse=" << c.normalImpulse
                            << " tangentImpulse=(" << c.tangentImpulse.x << "," << c.tangentImpulse.y << ")"
                            << " tangentMag=" << glm::length(c.tangentImpulse)
                            << " staticCoeff=" << staticCoeff << " kineticCoeff=" << kineticCoeff;
                        LOGGING::ECX_Logger::GetInstance()->LogMessage(oss.str(), LOGGING::LogLevel::INFORMATION);
                    }
                }
            }
        }

        // --- APPLY: every recorded body's total accumulated impulse
        // becomes a real velocity change, exactly once, now that every
        // pass and every contact point is done. A sleeping body that
        // genuinely got pushed (real resulting speed, not just numerical
        // noise) wakes up right here, so it's picked up by the gravity/
        // integration loop below in the same tick instead of losing a
        // frame reacting to it. ---
        for (auto& [entity, body] : bodies) {
            if (body.invMass <= 0.0f) continue;
            if (!manager.hasComponent<EC_DOD_Spatial>(entity)) continue;
            auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);
            spatial.velocity += body.accumulatedLinearImpulse * body.invMass;
            spatial.angVelocity += body.invInertiaWorld * body.accumulatedAngularImpulse;

            if (manager.hasComponent<EC_DOD_RigidBody>(entity)) {
                auto& rb = manager.getComponent<EC_DOD_RigidBody>(entity);
                if (rb.isSleeping &&
                    (glm::dot(spatial.velocity, spatial.velocity) > 1e-6f ||
                     glm::dot(spatial.angVelocity, spatial.angVelocity) > 1e-6f)) {
                    rb.isSleeping = false;
                    rb.sleepTimer = 0.0f;
                }
            }
        }

        // --- Positional correction: resolves a fraction of any remaining
        // penetration beyond a small slop, translation only, once per
        // still-colliding pair, after velocity has converged above. ---
        for (const EC_CollisionPair& pair : EC_PairManager::getAllPairs()) {
            if (!pair.m_Colliding) continue;
            if (pair.m_PenetrationDepth <= kPenetrationSlop) continue;

            float invMassA = 0.0f, invMassB = 0.0f;
            if (manager.hasComponent<EC_DOD_RigidBody>(pair.body_A)) {
                const auto& rb = manager.getComponent<EC_DOD_RigidBody>(pair.body_A);
                if (!rb.isStatic && rb.mass > 1e-6f) invMassA = 1.0f / rb.mass;
            }
            if (manager.hasComponent<EC_DOD_RigidBody>(pair.body_B)) {
                const auto& rb = manager.getComponent<EC_DOD_RigidBody>(pair.body_B);
                if (!rb.isStatic && rb.mass > 1e-6f) invMassB = 1.0f / rb.mass;
            }
            const float invMassSum = invMassA + invMassB;
            if (invMassSum <= 1e-6f) continue;

            if (!manager.hasComponent<EC_DOD_Spatial>(pair.body_A) ||
                !manager.hasComponent<EC_DOD_Spatial>(pair.body_B)) continue;
            auto& spatialA = manager.getComponent<EC_DOD_Spatial>(pair.body_A);
            auto& spatialB = manager.getComponent<EC_DOD_Spatial>(pair.body_B);

            const float excessDepth = pair.m_PenetrationDepth - kPenetrationSlop;
            const float correctionMag = excessDepth / invMassSum * kPositionCorrectionPercent;
            const glm::vec3 correction = correctionMag * pair.m_ContactNormal;

            if (invMassA > 0.0f) spatialA.position -= correction * invMassA;
            if (invMassB > 0.0f) spatialB.position += correction * invMassB;
        }
    }

    // --- Damping, sleep, integration: every awake, non-static rigid body,
    // once per tick. Gravity already applied above, before the solve. ---
    auto* rbArray = manager.getComponentArray<EC_DOD_RigidBody>();
    if (!rbArray) return;

    std::shared_lock lock(rbArray->getMutex());
    auto& rigidBodies = rbArray->getData();

    float totalMechanicalEnergy = 0.0f;

    for (size_t i = 0; i < rigidBodies.size(); i++) {
        auto& rb = rigidBodies[i];
        EntityID entity = rbArray->getEntity(i);

        if (rb.isStatic) continue;
        if (!manager.hasComponent<EC_DOD_Spatial>(entity)) continue;
        auto& spatial = manager.getComponent<EC_DOD_Spatial>(entity);

        if (rb.isSleeping) continue;

        spatial.velocity *= std::max(0.0f, 1.0f - rb.linearDamping * deltaTimeS);
        spatial.angVelocity *= std::max(0.0f, 1.0f - rb.angularDamping * deltaTimeS);

        const glm::mat3 invInertiaWorld = EC_PhysicsResolution::computeInvInertiaWorld(entity, rb.mass);

        const glm::vec3 linearMomentum = rb.mass * spatial.velocity;
        glm::mat3 inertiaWorld(0.0f);
        if (std::abs(glm::determinant(invInertiaWorld)) > 1e-8f) {
            inertiaWorld = glm::inverse(invInertiaWorld);
        }
        const glm::vec3 angularMomentum = inertiaWorld * spatial.angVelocity;

        if (m_LogEnergy) {
            const float kineticEnergy = 0.5f * rb.mass * glm::dot(spatial.velocity, spatial.velocity);
            const float rotationalEnergy = 0.5f * glm::dot(spatial.angVelocity, angularMomentum);
            const float potentialEnergy = -rb.mass * kGravity.y * spatial.position.y;
            totalMechanicalEnergy += kineticEnergy + rotationalEnergy + potentialEnergy;
        }

        if ((m_LogVelocity || m_LogAngularVelocity) && ++m_BodyLogCounter >= kDebugLogInterval) {
            m_BodyLogCounter = 0;
            if (m_LogVelocity) {
                std::ostringstream oss;
                oss << "[VELOCITY] entity=" << entity
                    << " velocity=(" << spatial.velocity.x << "," << spatial.velocity.y << "," << spatial.velocity.z << ")"
                    << " speed=" << glm::length(spatial.velocity)
                    << " pos=(" << spatial.position.x << "," << spatial.position.y << "," << spatial.position.z << ")";
                LOGGING::ECX_Logger::GetInstance()->LogMessage(oss.str(), LOGGING::LogLevel::INFORMATION);
            }
            if (m_LogAngularVelocity) {
                std::ostringstream oss;
                oss << "[ANGULAR_VELOCITY] entity=" << entity
                    << " angVelocity=(" << spatial.angVelocity.x << "," << spatial.angVelocity.y << "," << spatial.angVelocity.z << ")"
                    << " speed=" << glm::length(spatial.angVelocity);
                LOGGING::ECX_Logger::GetInstance()->LogMessage(oss.str(), LOGGING::LogLevel::INFORMATION);
            }
        }

        const bool atRest =
            glm::dot(linearMomentum, linearMomentum) < kSleepLinearMomentumThreshold * kSleepLinearMomentumThreshold &&
            glm::dot(angularMomentum, angularMomentum) < kSleepAngularMomentumThreshold * kSleepAngularMomentumThreshold;

        rb.sleepTimer = atRest ? (rb.sleepTimer + deltaTimeS) : 0.0f;
        if (rb.sleepTimer >= kTimeToSleep) {
            rb.isSleeping = true;
            spatial.velocity = glm::vec3(0.0f);
            spatial.angVelocity = glm::vec3(0.0f);
            continue;
        }

        // --- Integrate position/orientation from the final velocity.
        // Angular velocity is composed as a real quaternion rotation, not
        // summed component-wise into Euler angles and rebuilt via three
        // sequential single-axis rotations - that only tracks rotation
        // correctly about one fixed axis at a time. ---
        spatial.position += spatial.velocity * deltaTimeS;

        const glm::mat3 basis(spatial.right, spatial.up, -spatial.direction);
        const glm::quat currentQuat = glm::normalize(glm::quat_cast(basis));

        const float angSpeed = glm::length(spatial.angVelocity);
        glm::quat deltaQuat(1.0f, 0.0f, 0.0f, 0.0f);
        if (angSpeed > 1e-8f) {
            deltaQuat = glm::angleAxis(angSpeed * deltaTimeS, spatial.angVelocity / angSpeed);
        }
        const glm::quat newQuat = glm::normalize(deltaQuat * currentQuat);
        spatial.orientationQuat = newQuat;

        const glm::mat3 newBasis = glm::mat3_cast(newQuat);
        spatial.right = glm::normalize(newBasis[0]);
        spatial.up = glm::normalize(newBasis[1]);
        spatial.direction = glm::normalize(-newBasis[2]);

        glm::extractEulerAngleXYZ(glm::mat4(newBasis),
            spatial.orientation.x, spatial.orientation.y, spatial.orientation.z);
    }

    if (m_LogEnergy) {
        std::ostringstream oss;
        oss << "[ENERGY] " << totalMechanicalEnergy;
        LOGGING::ECX_Logger::GetInstance()->LogMessage(oss.str(), LOGGING::LogLevel::INFORMATION);
    }
}
