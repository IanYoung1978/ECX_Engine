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
		// Only populated (non-null shared_ptrs) when collider.type == Mesh - see
		// EC_DOD_MeshCollisionData's own comment. Cheap to snapshot alongside
		// collider/spatial above since it's just three shared_ptr copies (refcount
		// bumps), not the underlying mesh data itself.
		EC_DOD_MeshCollisionData meshCollisionData;
	};

	// castsShadow is looked up live, only for candidates that survive the coarse filters in
	// castRay/handleConeCheck, rather than cached in the per-tick snapshot.
	bool entityCastsShadow(EntityID entity) const;

	AABB computeWorldAABB(const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial);
	ECXResponse handleFrustumCheck(ECXRequest& request);
	ECXResponse handleEntitySearch(ECXRequest& request);
	ECXResponse handleRayCheck(ECXRequest& request);
	ECXResponse handleConeCheck(ECXRequest& request);
	ECXResponse handleCapsuleCheck(ECXRequest& request);
	// Shared broad+precise ray logic, used by handleRayCheck and by handleConeCheck for its
	// line-of-sight test.
	std::vector<RayQueryHit> castRay(const glm::vec3& origin, const glm::vec3& dir, float maxDistance,
		uint32_t layerMask, bool requireCastsShadow, bool firstHitOnly);
	// Capsule-vs-scene overlap using the real per-type tests (Mesh included). RayQueryHit::distance
	// holds penetration depth and normal points away from the capsule. excludeEntity skips one
	// candidate so a capsule doesn't hit itself.
	std::vector<RayQueryHit> castCapsule(const glm::vec3& pointA, const glm::vec3& pointB, float radius,
		uint32_t layerMask, bool firstHitOnly, EntityID excludeEntity = INVALID_ENTITY);

	EC_PairManager m_PairManager;

	// Per-tick snapshot of every Collider+Spatial entity. Built on the physics thread, read by
	// receive() on the main thread; m_Mutex guards the members below and the writer swaps in under
	// a short lock.
	std::mutex m_Mutex;
	std::vector<EntityAABB> m_EntityAABBs;
	std::unordered_map<uint32_t, size_t> m_EntityIndex; // entityId -> index into m_EntityAABBs
	EC_SpatialGrid m_SpatialGrid;
};