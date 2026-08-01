#include "EC_NarrowPhase.h"
#include "EC_CollisionChecks.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include "EC_CollisionChecks.h"
#include "EC_PhysicsResolution.h"
#include "Messaging/ECXEvent.h"
#include "Messaging/ECXMessenger.h"
#include <functional>
#include "Logging/ECX_Logging.h"
void EC_NarrowPhase::init(ECXMessenger& messenger)
{
    m_Messenger = &messenger;
    initCollisionDispatchTable();  // Add this line
}

// Purely geometric: detects overlap and caches the resulting manifold
// (contact points, normal, penetration depth) on the pair for the physics
// system to consume later this tick. Computes no impulses/forces at all -
// that's entirely the physics system's job (see EC_PhysicsSystem), which
// iterates the same pairs afterward via EC_PairManager::getAllPairs().
void EC_NarrowPhase::narrowPhaseCollisionDetection()
{
    // For each pair in the pair manager, perform narrow phase collision detection
    while (m_PairManager.hasPairs())
    {
        EC_CollisionPair& pair = m_PairManager.getNextPair();

        // Copy components to avoid holding locks during collision checks
        EC_DOD_Collider colliderA, colliderB;
        EC_DOD_Spatial spatialA, spatialB;

        try {
            // Get entities from IDs, from entity manager
            colliderA = EC_DOD_EntityManager::getInstance().getComponent<EC_DOD_Collider>(pair.body_A);
            colliderB = EC_DOD_EntityManager::getInstance().getComponent<EC_DOD_Collider>(pair.body_B);

            // Get spatial components for positions
            spatialA = EC_DOD_EntityManager::getInstance().getComponent<EC_DOD_Spatial>(pair.body_A);
            spatialB = EC_DOD_EntityManager::getInstance().getComponent<EC_DOD_Spatial>(pair.body_B);
        }
        catch (const std::runtime_error&) {
            // Entity was destroyed or doesn't have required components
            pair.m_Colliding = false;
            pair.m_CollisionPoints.clear();
            continue;
        }

        // Prepare collision manifold
        CollisionManifold manifold;
        bool collisionDetected = false;

        // Perform collision detection based on collider types
        collisionDetected = performCollisionCheck(colliderA, spatialA, colliderB, spatialB, manifold);

        // Handle collision state changes
        handleCollisionStateChange(pair, collisionDetected, manifold);

        // Update pair collision status
        pair.m_Colliding = collisionDetected;

        // If collision detected, cache the geometric manifold on the pair -
        // contact points, normal, and penetration depth - for the physics
        // system to read afterward. No eligibility/force decisions here.
        if (collisionDetected)
        {
            pair.m_CollisionPoints = manifold.contactPoints;
            pair.m_ContactNormal = manifold.contactNormal;
            pair.m_PenetrationDepth = manifold.penetrationDepth;
        }
        else
        {
            pair.m_CollisionPoints.clear();
            pair.m_ContactNormal = glm::vec3(0.0f);
            pair.m_PenetrationDepth = 0.0f;
        }
    }
}

// Collision function signature
using CollisionCheckFunc = std::function<bool(
    const EC_DOD_Collider&, const EC_DOD_Spatial&,
    const EC_DOD_Collider&, const EC_DOD_Spatial&,
    CollisionManifold&)>;

void EC_NarrowPhase::initCollisionDispatchTable()
{
    // Initialize collision dispatch table
    m_CollisionDispatch = {
        // Sphere vs Sphere
        {makeKey(EC_DOD_Collider::Type::Sphere, EC_DOD_Collider::Type::Sphere),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                return EC_CollisionChecks::SphereVsSphere(
                    Sphere{a.center, a.radius}, sa.position,
                    Sphere{b.center, b.radius}, sb.position, m);
            }},

        // AABB vs AABB
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                return EC_CollisionChecks::AABBVsAABB(
                    AABB{a.center - a.extents, a.center + a.extents}, sa.position,
                    AABB{b.center - b.extents, b.center + b.extents}, sb.position, m);
            }},

        // Sphere vs AABB
        {makeKey(EC_DOD_Collider::Type::Sphere, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                bool hit = EC_CollisionChecks::SphereVsAABB(
                    Sphere{a.center, a.radius}, sa.position,
                    AABB{b.center - b.extents, b.center + b.extents}, sb.position, m);
                // SphereVsAABB's normal points toward the sphere (param A here); flip
                // so contactNormal consistently points body_A -> body_B like every
                // other dispatch entry.
                if (hit) m.contactNormal = -m.contactNormal;
                return hit;
            }},

        // AABB vs Sphere (swap parameters)
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::Sphere),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                return EC_CollisionChecks::SphereVsAABB(
                    Sphere{b.center, b.radius}, sb.position,
                    AABB{a.center - a.extents, a.center + a.extents}, sa.position, m);
            }},

        // OBB vs OBB
        {makeKey(EC_DOD_Collider::Type::OBB, EC_DOD_Collider::Type::OBB),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                glm::mat3 orientA = glm::mat3(glm::normalize(sa.right),
                                             glm::normalize(sa.up),
                                             glm::normalize(sa.direction));
                glm::mat3 orientB = glm::mat3(glm::normalize(sb.right),
                                             glm::normalize(sb.up),
                                             glm::normalize(sb.direction));
                return EC_CollisionChecks::OBBVsOBB(
                    OBB{a.center, a.extents, orientA}, sa.position,
                    OBB{b.center, b.extents, orientB}, sb.position, m);
            }},

        // OBB vs AABB (AABB never rotates, so treat it as an identity-oriented
        // OBB and reuse the same SAT manifold code - OBBVsOBB already
        // self-orients its normal body_A -> body_B, no extra sign-flip needed
        // here unlike the Sphere pairs above).
        {makeKey(EC_DOD_Collider::Type::OBB, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                glm::mat3 orientA = glm::mat3(glm::normalize(sa.right),
                                             glm::normalize(sa.up),
                                             glm::normalize(sa.direction));
                return EC_CollisionChecks::OBBVsOBB(
                    OBB{a.center, a.extents, orientA}, sa.position,
                    OBB{b.center, b.extents, glm::mat3(1.0f)}, sb.position, m);
            }},

        // AABB vs OBB (swap parameters)
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::OBB),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                glm::mat3 orientB = glm::mat3(glm::normalize(sb.right),
                                             glm::normalize(sb.up),
                                             glm::normalize(sb.direction));
                return EC_CollisionChecks::OBBVsOBB(
                    OBB{a.center, a.extents, glm::mat3(1.0f)}, sa.position,
                    OBB{b.center, b.extents, orientB}, sb.position, m);
            }},

        // Sphere vs OBB
        {makeKey(EC_DOD_Collider::Type::Sphere, EC_DOD_Collider::Type::OBB),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                glm::mat3 orientB = glm::mat3(glm::normalize(sb.right),
                                             glm::normalize(sb.up),
                                             glm::normalize(sb.direction));
                bool hit = EC_CollisionChecks::SphereVsOBB(
                    Sphere{a.center, a.radius}, sa.position,
                    OBB{b.center, b.extents, orientB}, sb.position, m);
                // Same fix as Sphere vs AABB above: flip to body_A -> body_B convention.
                if (hit) m.contactNormal = -m.contactNormal;
                return hit;
            }},

        // OBB vs Sphere (swap parameters)
        {makeKey(EC_DOD_Collider::Type::OBB, EC_DOD_Collider::Type::Sphere),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                glm::mat3 orientA = glm::mat3(glm::normalize(sa.right),
                                             glm::normalize(sa.up),
                                             glm::normalize(sa.direction));
                return EC_CollisionChecks::SphereVsOBB(
                    Sphere{b.center, b.radius}, sb.position,
                    OBB{a.center, a.extents, orientA}, sa.position, m);
            }},

        // Frustum vs AABB (no manifold for frustum checks)
        {makeKey(EC_DOD_Collider::Type::Frustum, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                return EC_CollisionChecks::FrustumVsAABB(
                    Frustum{sa.position, sa.direction, sa.up, sa.right,
                           a.radius, a.height, 60.0f, 1.77f}, sa.position,
                    AABB{b.center - b.extents, b.center + b.extents}, sb.position);
            }},

        // AABB vs Frustum (swap parameters)
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::Frustum),
            [](auto& a, auto& sa, auto& b, auto& sb, auto& m) {
                return EC_CollisionChecks::FrustumVsAABB(
                    Frustum{sb.position, sb.direction, sb.up, sb.right,
                           b.radius, b.height, 60.0f, 1.77f}, sb.position,
                    AABB{a.center - a.extents, a.center + a.extents}, sa.position);
            }},
    };
}

bool EC_NarrowPhase::performCollisionCheck(
    const EC_DOD_Collider& colliderA, const EC_DOD_Spatial& spatialA,
    const EC_DOD_Collider& colliderB, const EC_DOD_Spatial& spatialB,
    CollisionManifold& manifold)
{
    // Create key from collider types
    uint64_t key = makeKey(colliderA.type, colliderB.type);

    // Look up collision function
    auto it = m_CollisionDispatch.find(key);
    if (it != m_CollisionDispatch.end())
    {
        return it->second(colliderA, spatialA, colliderB, spatialB, manifold);
    }

    // Unsupported collision type combination
    return false;
}

void EC_NarrowPhase::handleCollisionStateChange(
    EC_CollisionPair& pair,
    bool collisionDetected,
    const CollisionManifold& manifold)
{
    // New collision detected (collision begin)
    if (!pair.m_Colliding && collisionDetected)
    {
        ECXEvent msg;
        msg.type = ECXEventType::CollisionBeginEvent;
        msg.args[0] = manifold;
        msg.args[1] = pair.body_A;
        msg.args[2] = pair.body_B;
        m_Messenger->publish(msg);
    }

    // Collision ended
    if (pair.m_Colliding && !collisionDetected)
    {
        // Either side may have been sleeping while resting against the
        // other (e.g. a cube asleep on top of one that just got knocked
        // away) - losing that contact needs to wake it back up rather than
        // leaving it frozen mid-air with nothing left holding it up.
        EC_PhysicsResolution::wakeIfSleeping(pair.body_A);
        EC_PhysicsResolution::wakeIfSleeping(pair.body_B);

        ECXEvent msg;
        msg.type = ECXEventType::CollisionEndEvent;
        msg.args[0] = pair.body_A;
        msg.args[1] = pair.body_B;
        m_Messenger->publish(msg);
    }
}