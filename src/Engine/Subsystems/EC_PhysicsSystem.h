#pragma once
#include "EC_System.h"
#include "Entity/EC_DOD_Types.h"
#include <vector>

class EC_PairManager;

// Runs sequentially, on the same thread, right after EC_CollisionSystem -
// collision detection only ever finds intersections and caches their
// geometric manifold (contact points, normal, penetration depth) on the
// pair; it computes no forces/impulses at all. This system owns all of
// that: given the pair manager it's wired to at startup (setPairManager,
// called once from EC_Engine::init), each tick it -
//   1. iterates every currently-colliding pair, computing and caching the
//      collision-related linear/angular impulse for both bodies involved
//      (EC_PhysicsResolution::accumulateImpulses) into each body's
//      EC_DOD_ImpulseAccumulator;
//   2. then, for every awake non-static EC_DOD_RigidBody, applies gravity
//      plus that cached accumulator (linear momentum first, then angular),
//      then damping, then integrates position/orientation from the result
//      and resets the accumulator for next tick's step 1 to refill.
class EC_PhysicsSystem : public EC_System {
public:
    EC_PhysicsSystem();
    virtual ~EC_PhysicsSystem();

    virtual void init(ECXMessenger& messenger, EC_Game& game) override;
    virtual void update(const float& deltaTimeS, EC_Game& game) override;

    // Wired once at startup (EC_Engine::init) to the same EC_PairManager
    // instance EC_CollisionSystem's broad/narrow phase populate.
    void setPairManager(EC_PairManager* pairManager) { m_PairManager = pairManager; }

    // Wired once at startup (EC_Engine::init, from EngineConfig.xml's
    // <Physics><Debug><LogEnergy> flag) - when set, logs every tick's total
    // system mechanical energy (kinetic + gravitational potential, summed
    // across every awake dynamic body) via ECX_Logger. That's the correct
    // invariant to watch for a real solver bug: it should only ever
    // decrease (friction/damping/inelastic impacts dissipate it) or drop
    // sharply when a body goes to sleep (excluded from the sum) - any
    // sustained increase means the solver is injecting energy that
    // shouldn't be there, as opposed to kinetic energy alone, which
    // legitimately grows anytime something is simply falling.
    void setLogEnergy(bool enabled) { m_LogEnergy = enabled; }

private:
    EC_PairManager* m_PairManager = nullptr;
    bool m_LogEnergy = false;
    // Entities that carried an EC_DOD_DebugContacts component last tick -
    // diffed against this tick's contact set so entities that stop colliding
    // get their debug contacts cleared instead of showing stale points.
    std::vector<EntityID> m_LastDebugContactEntities;
};
