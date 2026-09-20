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

        // Only Type::Mesh entities (e.g. voxel terrain chunks) ever have this - default-
        // constructed (null shared_ptrs) for everything else, which the Mesh-involving
        // dispatch entries below check for before dereferencing.
        EC_DOD_MeshCollisionData meshDataA, meshDataB;
        auto& entityManager = EC_DOD_EntityManager::getInstance();
        if (entityManager.hasComponent<EC_DOD_MeshCollisionData>(pair.body_A))
            meshDataA = entityManager.getComponent<EC_DOD_MeshCollisionData>(pair.body_A);
        if (entityManager.hasComponent<EC_DOD_MeshCollisionData>(pair.body_B))
            meshDataB = entityManager.getComponent<EC_DOD_MeshCollisionData>(pair.body_B);

        // A Kinematic body moving while already mid-contact (e.g. walking across uneven
        // terrain) needs a fresh BEGIN to give script another chance to react to the new
        // contact - Dynamic bodies don't need this at all, since EC_PhysicsSystem already
        // re-resolves every currently-colliding pair every tick regardless of BEGIN/END.
        // See EC_CollisionPair's own comment on the fields this reads/writes.
        auto isKinematic = [&entityManager](EntityID e) {
            return entityManager.hasComponent<EC_DOD_RigidBody>(e) &&
                entityManager.getComponent<EC_DOD_RigidBody>(e).bodyType == EC_BodyType::Kinematic;
        };
        bool wasCollidingBeforeMoveCheck = pair.m_Colliding;
        bool moved = false;
        if (wasCollidingBeforeMoveCheck && (isKinematic(pair.body_A) || isKinematic(pair.body_B))) {
            moved = spatialA.position != pair.m_LastCheckedPositionA ||
                spatialA.orientation != pair.m_LastCheckedOrientationA ||
                spatialB.position != pair.m_LastCheckedPositionB ||
                spatialB.orientation != pair.m_LastCheckedOrientationB;
        }
        pair.m_LastCheckedPositionA = spatialA.position;
        pair.m_LastCheckedOrientationA = spatialA.orientation;
        pair.m_LastCheckedPositionB = spatialB.position;
        pair.m_LastCheckedOrientationB = spatialB.orientation;

        // Prepare collision manifold
        CollisionManifold manifold;
        bool collisionDetected = false;

        // Perform collision detection based on collider types
        collisionDetected = performCollisionCheck(colliderA, spatialA, meshDataA, colliderB, spatialB, meshDataB, manifold);

        // Only fake a false->true transition (to re-fire BEGIN with the fresh manifold) if
        // the pair is genuinely STILL colliding after moving - a real loss of contact (moved
        // off the edge of the surface) must fall through to handleCollisionStateChange's
        // normal true->false path below and fire a real END, not get swallowed here.
        if (moved && collisionDetected) {
            pair.m_Colliding = false;
        }

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
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                return EC_CollisionChecks::SphereVsSphere(
                    Sphere{a.center, a.radius}, sa.position,
                    Sphere{b.center, b.radius}, sb.position, m);
            }},

        // AABB vs AABB
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                return EC_CollisionChecks::AABBVsAABB(
                    AABB{a.center - a.extents, a.center + a.extents}, sa.position,
                    AABB{b.center - b.extents, b.center + b.extents}, sb.position, m);
            }},

        // Sphere vs AABB
        {makeKey(EC_DOD_Collider::Type::Sphere, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
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
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                return EC_CollisionChecks::SphereVsAABB(
                    Sphere{b.center, b.radius}, sb.position,
                    AABB{a.center - a.extents, a.center + a.extents}, sa.position, m);
            }},

        // OBB vs OBB
        {makeKey(EC_DOD_Collider::Type::OBB, EC_DOD_Collider::Type::OBB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
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
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::mat3 orientA = glm::mat3(glm::normalize(sa.right),
                                             glm::normalize(sa.up),
                                             glm::normalize(sa.direction));
                return EC_CollisionChecks::OBBVsOBB(
                    OBB{a.center, a.extents, orientA}, sa.position,
                    OBB{b.center, b.extents, glm::mat3(1.0f)}, sb.position, m);
            }},

        // AABB vs OBB (swap parameters)
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::OBB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::mat3 orientB = glm::mat3(glm::normalize(sb.right),
                                             glm::normalize(sb.up),
                                             glm::normalize(sb.direction));
                return EC_CollisionChecks::OBBVsOBB(
                    OBB{a.center, a.extents, glm::mat3(1.0f)}, sa.position,
                    OBB{b.center, b.extents, orientB}, sb.position, m);
            }},

        // Sphere vs OBB
        {makeKey(EC_DOD_Collider::Type::Sphere, EC_DOD_Collider::Type::OBB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
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
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::mat3 orientA = glm::mat3(glm::normalize(sa.right),
                                             glm::normalize(sa.up),
                                             glm::normalize(sa.direction));
                return EC_CollisionChecks::SphereVsOBB(
                    Sphere{b.center, b.radius}, sb.position,
                    OBB{a.center, a.extents, orientA}, sa.position, m);
            }},

        // Frustum vs AABB (no manifold for frustum checks)
        {makeKey(EC_DOD_Collider::Type::Frustum, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                return EC_CollisionChecks::FrustumVsAABB(
                    Frustum{sa.position, sa.direction, sa.up, sa.right,
                           a.radius, a.height, 60.0f, 1.77f}, sa.position,
                    AABB{b.center - b.extents, b.center + b.extents}, sb.position);
            }},

        // AABB vs Frustum (swap parameters)
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::Frustum),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                return EC_CollisionChecks::FrustumVsAABB(
                    Frustum{sb.position, sb.direction, sb.up, sb.right,
                           b.radius, b.height, 60.0f, 1.77f}, sb.position,
                    AABB{a.center - a.extents, a.center + a.extents}, sa.position);
            }},

        // Capsule vs Capsule - self-orienting (like SphereVsSphere), no flip needed.
        {makeKey(EC_DOD_Collider::Type::Capsule, EC_DOD_Collider::Type::Capsule),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::vec3 upA = glm::normalize(sa.up) * (a.height * 0.5f);
                glm::vec3 upB = glm::normalize(sb.up) * (b.height * 0.5f);
                return EC_CollisionChecks::CapsuleVsCapsule(
                    Capsule{a.center - upA, a.center + upA, a.radius}, sa.position,
                    Capsule{b.center - upB, b.center + upB, b.radius}, sb.position, m);
            }},

        // Capsule vs Sphere - CapsuleVsSphere's normal points toward its second
        // parameter (the sphere), which is dispatch-B here, so this is already the
        // correct A -> B direction; no flip.
        {makeKey(EC_DOD_Collider::Type::Capsule, EC_DOD_Collider::Type::Sphere),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::vec3 upA = glm::normalize(sa.up) * (a.height * 0.5f);
                return EC_CollisionChecks::CapsuleVsSphere(
                    Capsule{a.center - upA, a.center + upA, a.radius}, sa.position,
                    Sphere{b.center, b.radius}, sb.position, m);
            }},

        // Sphere vs Capsule (swap parameters) - the sphere is now CapsuleVsSphere's
        // second parameter (so the normal points toward it) but it's dispatch-A, so
        // this is backwards (B -> A) and must be flipped.
        {makeKey(EC_DOD_Collider::Type::Sphere, EC_DOD_Collider::Type::Capsule),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::vec3 upB = glm::normalize(sb.up) * (b.height * 0.5f);
                bool hit = EC_CollisionChecks::CapsuleVsSphere(
                    Capsule{b.center - upB, b.center + upB, b.radius}, sb.position,
                    Sphere{a.center, a.radius}, sa.position, m);
                if (hit) m.contactNormal = -m.contactNormal;
                return hit;
            }},

        // Capsule vs AABB - normal points toward the AABB (CapsuleVsAABB's second
        // parameter), which is dispatch-B here - correct A -> B direction as-is.
        {makeKey(EC_DOD_Collider::Type::Capsule, EC_DOD_Collider::Type::AABB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::vec3 upA = glm::normalize(sa.up) * (a.height * 0.5f);
                return EC_CollisionChecks::CapsuleVsAABB(
                    Capsule{a.center - upA, a.center + upA, a.radius}, sa.position,
                    AABB{b.center - b.extents, b.center + b.extents}, sb.position, m);
            }},

        // AABB vs Capsule (swap parameters) - the AABB is now CapsuleVsAABB's second
        // parameter (so the normal points toward it) but it's dispatch-A, so this is
        // backwards (B -> A) and must be flipped.
        {makeKey(EC_DOD_Collider::Type::AABB, EC_DOD_Collider::Type::Capsule),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::vec3 upB = glm::normalize(sb.up) * (b.height * 0.5f);
                bool hit = EC_CollisionChecks::CapsuleVsAABB(
                    Capsule{b.center - upB, b.center + upB, b.radius}, sb.position,
                    AABB{a.center - a.extents, a.center + a.extents}, sa.position, m);
                if (hit) m.contactNormal = -m.contactNormal;
                return hit;
            }},

        // Capsule vs OBB - normal points toward the OBB (CapsuleVsOBB's second
        // parameter), which is dispatch-B here - correct A -> B direction as-is.
        {makeKey(EC_DOD_Collider::Type::Capsule, EC_DOD_Collider::Type::OBB),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::vec3 upA = glm::normalize(sa.up) * (a.height * 0.5f);
                glm::mat3 orientB = glm::mat3(glm::normalize(sb.right),
                                             glm::normalize(sb.up),
                                             glm::normalize(sb.direction));
                return EC_CollisionChecks::CapsuleVsOBB(
                    Capsule{a.center - upA, a.center + upA, a.radius}, sa.position,
                    OBB{b.center, b.extents, orientB}, sb.position, m);
            }},

        // OBB vs Capsule (swap parameters) - the OBB is now CapsuleVsOBB's second
        // parameter (so the normal points toward it) but it's dispatch-A, so this is
        // backwards (B -> A) and must be flipped.
        {makeKey(EC_DOD_Collider::Type::OBB, EC_DOD_Collider::Type::Capsule),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                glm::vec3 upB = glm::normalize(sb.up) * (b.height * 0.5f);
                glm::mat3 orientA = glm::mat3(glm::normalize(sa.right),
                                             glm::normalize(sa.up),
                                             glm::normalize(sa.direction));
                bool hit = EC_CollisionChecks::CapsuleVsOBB(
                    Capsule{b.center - upB, b.center + upB, b.radius}, sb.position,
                    OBB{a.center, a.extents, orientA}, sa.position, m);
                if (hit) m.contactNormal = -m.contactNormal;
                return hit;
            }},

        // Capsule vs Mesh - real triangle-level collision (e.g. voxel terrain), reusing
        // the same math EC_BroadPhase::castCapsule already uses for one-shot queries (see
        // EC_CollisionChecks::CapsuleVsMesh). Its contactNormal is the winning triangle's
        // own absolute surface normal, not relative to dispatch order like the primitive-
        // vs-primitive entries above - no flip needed in either direction below.
        {makeKey(EC_DOD_Collider::Type::Capsule, EC_DOD_Collider::Type::Mesh),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                if (!mdb.positions || !mdb.normals || !mdb.indices) return false;
                glm::vec3 upA = glm::normalize(sa.up) * (a.height * 0.5f);
                return EC_CollisionChecks::CapsuleVsMesh(
                    Capsule{a.center - upA, a.center + upA, a.radius}, sa.position,
                    *mdb.positions, *mdb.normals, *mdb.indices, sb.position, m);
            }},

        // Mesh vs Capsule (swap parameters) - same underlying test; the mesh's own
        // absolute surface normal still needs no flip regardless of dispatch order.
        {makeKey(EC_DOD_Collider::Type::Mesh, EC_DOD_Collider::Type::Capsule),
            [](auto& a, auto& sa, auto& mda, auto& b, auto& sb, auto& mdb, auto& m) {
                if (!mda.positions || !mda.normals || !mda.indices) return false;
                glm::vec3 upB = glm::normalize(sb.up) * (b.height * 0.5f);
                return EC_CollisionChecks::CapsuleVsMesh(
                    Capsule{b.center - upB, b.center + upB, b.radius}, sb.position,
                    *mda.positions, *mda.normals, *mda.indices, sa.position, m);
            }},
    };
}

bool EC_NarrowPhase::performCollisionCheck(
    const EC_DOD_Collider& colliderA, const EC_DOD_Spatial& spatialA, const EC_DOD_MeshCollisionData& meshDataA,
    const EC_DOD_Collider& colliderB, const EC_DOD_Spatial& spatialB, const EC_DOD_MeshCollisionData& meshDataB,
    CollisionManifold& manifold)
{
    // Create key from collider types
    uint64_t key = makeKey(colliderA.type, colliderB.type);

    // Look up collision function
    auto it = m_CollisionDispatch.find(key);
    if (it != m_CollisionDispatch.end())
    {
        return it->second(colliderA, spatialA, meshDataA, colliderB, spatialB, meshDataB, manifold);
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