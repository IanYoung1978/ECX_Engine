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
#include "Spatial/RayQueryHit.h"

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
		// Raw collider/spatial (not just the derived AABB) so ray/cone queries can run a
		// precise per-shape test entirely from this mutex-protected snapshot, instead of
		// reading live component arrays from another thread at request time.
		EC_DOD_Collider collider;
		EC_DOD_Spatial spatial;
	};

	// castsShadow (EC_DOD_GraphicsData::castsShadow) is looked up live, on demand, only for
	// candidates that already survived the coarse spatial/angle filters in castRay/
	// handleConeCheck - NOT cached per-entity in broadPhaseCollisionDetection()'s per-tick
	// snapshot. That loop runs on the physics thread's unthrottled busy-spin
	// (EC_PhysicsThreadTask::execute() has no sleep), so adding a second component-array
	// lookup there for every collider entity, every tick, regardless of whether anything is
	// even querying, was measurable unwanted overhead on a hot path - not worth it for a
	// property only ray/cone queries ever consult.
	bool entityCastsShadow(EntityID entity) const;

	AABB computeWorldAABB(const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial);
	ECXResponse handleFrustumCheck(ECXRequest& request);
	ECXResponse handleEntitySearch(ECXRequest& request);
	ECXResponse handleRayCheck(ECXRequest& request);
	ECXResponse handleConeCheck(ECXRequest& request);
	// Shared broad+precise ray logic used directly by handleRayCheck and, for the
	// "unobstructed line-of-sight to apex" test, by handleConeCheck - the code-level link
	// satisfying Issue #29's stated dependency on Issue #30.
	std::vector<RayQueryHit> castRay(const glm::vec3& origin, const glm::vec3& dir, float maxDistance,
		uint32_t layerMask, bool requireCastsShadow, bool firstHitOnly);

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