#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include "Messaging/IRequestResponder.h"
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"
#include "Components/EC_DOD_Components.h"
#include "EC_CollisionShapes.h"
#include "Spatial/EC_SpatialGrid.h"

class EC_Game;
class ECXMessenger;

class EC_BroadPhase:
	public IRequestResponder
{
	public:
	EC_BroadPhase() = default;
	virtual ~EC_BroadPhase() = default;
	virtual void broadPhaseCollisionDetection();
	void init(ECXMessenger& messenger);
	// Inherited via IRequestResponder
	virtual ECXResponse receive(ECXRequest& request) override;

private:
	struct EntityAABB {
		uint32_t entityId;
		AABB worldAABB;
		uint32_t collisionLayer;
		uint32_t collisionMask;
	};

	AABB computeWorldAABB(const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial);
	ECXResponse handleFrustumCheck(ECXRequest& request);
	ECXResponse handleEntitySearch(ECXRequest& request);

	EC_PairManager m_PairManager;

	// Populated once per frame in broadPhaseCollisionDetection(), from every entity
	// with EC_DOD_Collider + EC_DOD_Spatial (not filtered to gameplay layers - that
	// filtering happens per-query in receive()). Backs both the spatial grid below and
	// direct layer-filtered scans.
	//
	// broadPhaseCollisionDetection() runs on the physics/scripting worker thread
	// (EC_PhysicsThreadTask), while receive() is called synchronously from the
	// renderer on the main thread - m_Mutex guards all three members below against
	// that cross-thread access. Write side builds into locals and swaps in under a
	// short lock, so the (potentially expensive) AABB computation itself doesn't hold
	// the lock.
	std::mutex m_Mutex;
	std::vector<EntityAABB> m_EntityAABBs;
	std::unordered_map<uint32_t, size_t> m_EntityIndex; // entityId -> index into m_EntityAABBs
	EC_SpatialGrid m_SpatialGrid;
};