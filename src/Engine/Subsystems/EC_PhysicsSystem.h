#pragma once
#include "EC_System.h"
#include "Entity/EC_DOD_Types.h"
#include <vector>

class EC_PairManager;

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
    // <Physics><Debug> flags) - one independent toggle per physics
    // attribute rather than a single switch, so e.g. friction can be
    // traced without a velocity line for every body every tick too.
    void setDebugLogging(bool logEnergy, bool logVelocity, bool logAngularVelocity, bool logFriction)
    {
        m_LogEnergy = logEnergy;
        m_LogVelocity = logVelocity;
        m_LogAngularVelocity = logAngularVelocity;
        m_LogFriction = logFriction;
    }

private:
    EC_PairManager* m_PairManager = nullptr;
    bool m_LogEnergy = false;
    bool m_LogVelocity = false;
    bool m_LogAngularVelocity = false;
    bool m_LogFriction = false;
    // Entities that carried an EC_DOD_DebugContacts component last tick -
    // diffed against this tick's contact set so entities that stop colliding
    // get their debug contacts cleared instead of showing stale points.
    std::vector<EntityID> m_LastDebugContactEntities;
    // Throttle counters for the (potentially very high frequency) debug
    // logs below - incremented once per body/contact-point per substep,
    // logging only every kDebugLogInterval-th one.
    int m_BodyLogCounter = 0;
    int m_ContactLogCounter = 0;
};
