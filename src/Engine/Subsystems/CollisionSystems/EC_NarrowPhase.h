#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"
#include "EC_CollisionShapes.h"
#include "Components/EC_DOD_Components.h"
#include <functional>

class ECXMessenger;


class EC_NarrowPhase
{
public:
    void init(ECXMessenger& messenger);
    void narrowPhaseCollisionDetection();

private:
    // Collision function signature
    using CollisionCheckFunc = std::function<bool(
        const EC_DOD_Collider&, const EC_DOD_Spatial&,
        const EC_DOD_Collider&, const EC_DOD_Spatial&,
        CollisionManifold&)>;

    void initCollisionDispatchTable();

    bool performCollisionCheck(
        const EC_DOD_Collider& colliderA, const EC_DOD_Spatial& spatialA,
        const EC_DOD_Collider& colliderB, const EC_DOD_Spatial& spatialB,
        CollisionManifold& manifold);

    void handleCollisionStateChange(
        EC_CollisionPair& pair,
        bool collisionDetected,
        const CollisionManifold& manifold);

    static inline uint64_t makeKey(EC_DOD_Collider::Type a, EC_DOD_Collider::Type b)
    {
        return (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
    }

    std::unordered_map<uint64_t, CollisionCheckFunc> m_CollisionDispatch;
    EC_PairManager m_PairManager;
    ECXMessenger* m_Messenger = nullptr;
};