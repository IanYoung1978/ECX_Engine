#include "EC_BroadPhase.h"
#include "Messaging/ECXMessenger.h"
#include "Messaging/ECXRequest.h"
#include "Messaging/ECXResponse.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "EC_CollisionShapes.h"
#include "EC_RayIntersection.h"
#include "EC_GJK.h"
#include "EC_ConvexSupport.h"
#include "Spatial/EC_Frustum.h"
#include <limits>
#include <cmath>

void EC_BroadPhase::broadPhaseCollisionDetection()
{
    // Get all entities with collider and spatial components
    auto entities = EC_DOD_EntityManager::getInstance().getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Collider)),
        std::type_index(typeid(EC_DOD_Spatial))
        });

    // Built entirely into locals - this function runs on the physics/scripting worker
    // thread, while receive() is called synchronously from the renderer on the main
    // thread. Doing the (potentially expensive) AABB computation and sweep-and-prune
    // here, unlocked, and only taking m_Mutex for the final swap-in keeps the critical
    // section short and avoids exposing partially-built state to the reader thread.
    std::vector<EntityAABB> localAABBs;
    std::unordered_map<uint32_t, size_t> localIndex;
    EC_SpatialGrid localGrid;

    if (!entities.empty()) {
        auto* colliderArray = EC_DOD_EntityManager::getInstance().getComponentArray<EC_DOD_Collider>();
        auto* spatialArray = EC_DOD_EntityManager::getInstance().getComponentArray<EC_DOD_Spatial>();

        if (colliderArray && spatialArray) {
            localAABBs.reserve(entities.size());

            for (uint32_t entityId : entities) {
                // A deactivated scene's entities (EC_GameScene::deactivate(), on
                // switching scenes) must drop out of the spatial index entirely - this
                // is the single point rendering (via queryVisibleEntities), gameplay
                // collision, and ray/cone queries all read from, so filtering here is
                // what actually stops a deactivated scene's geometry from continuing to
                // render/collide/hit-test after a scene switch. EC_DOD_EntityInfo::active
                // was previously written by scene activation but never read anywhere.
                if (EC_DOD_EntityManager::getInstance().hasComponent<EC_DOD_EntityInfo>(entityId) &&
                    !EC_DOD_EntityManager::getInstance().getComponent<EC_DOD_EntityInfo>(entityId).active)
                    continue;

                // Lock and get components (get() has its own locking)
                EC_DOD_Collider collider;
                EC_DOD_Spatial spatial;

                try {
                    collider = colliderArray->get(entityId);
                    spatial = spatialArray->get(entityId);
                }
                catch (const std::runtime_error&) {
                    // Entity doesn't have required components, skip
                    continue;
                }

                // Compute world-space AABB from collider bounds
                AABB worldAABB = computeWorldAABB(collider, spatial);

                localIndex[entityId] = localAABBs.size();
                localAABBs.push_back({ entityId, worldAABB, collider.collisionLayer, collider.collisionMask,
                    collider, spatial });
                localGrid.insert(entityId, worldAABB);
            }

            // Broad-phase collision detection using Sweep and Prune (Sort and Sweep)
            // This is efficient for many objects and handles temporal coherence well
            std::vector<EntityAABB> sweepOrder = localAABBs;

            // Sort entities by minimum X coordinate
            std::sort(sweepOrder.begin(), sweepOrder.end(),
                [](const EntityAABB& a, const EntityAABB& b) {
                    return a.worldAABB.min.x < b.worldAABB.min.x;
                });

            // Sweep and prune algorithm
            for (size_t i = 0; i < sweepOrder.size(); ++i) {
                const EntityAABB& entityA = sweepOrder[i];

                // Check against all entities whose min.x is less than entityA's max.x
                for (size_t j = i + 1; j < sweepOrder.size(); ++j) {
                    const EntityAABB& entityB = sweepOrder[j];

                    // Early exit if entityB is beyond entityA's max X
                    if (entityB.worldAABB.min.x > entityA.worldAABB.max.x)
                        break;

                    // Check collision layer filtering
                    bool canCollide = (entityA.collisionLayer & entityB.collisionMask) != 0 &&
                        (entityB.collisionLayer & entityA.collisionMask) != 0;

                    if (!canCollide)
                        continue;

                    // Check AABB overlap in all three axes
                    bool overlapX = entityA.worldAABB.max.x >= entityB.worldAABB.min.x &&
                        entityB.worldAABB.max.x >= entityA.worldAABB.min.x;
                    bool overlapY = entityA.worldAABB.max.y >= entityB.worldAABB.min.y &&
                        entityB.worldAABB.max.y >= entityA.worldAABB.min.y;
                    bool overlapZ = entityA.worldAABB.max.z >= entityB.worldAABB.min.z &&
                        entityB.worldAABB.max.z >= entityA.worldAABB.min.z;

                    if (overlapX && overlapY && overlapZ) {
                        // Potential collision detected - add to pair manager
                        m_PairManager.addPair(entityA.entityId, entityB.entityId);
                    }
                }
            }
        }
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_EntityAABBs = std::move(localAABBs);
    m_EntityIndex = std::move(localIndex);
    m_SpatialGrid = std::move(localGrid);
}



void EC_BroadPhase::init(ECXMessenger& messenger)
{
	messenger.Subscribe(*this, ECXRequestType::FrustumCheck);
	messenger.Subscribe(*this, ECXRequestType::EntitySearch);
	messenger.Subscribe(*this, ECXRequestType::RayCheck);
	messenger.Subscribe(*this, ECXRequestType::ConeCheck);
}

ECXResponse EC_BroadPhase::receive(ECXRequest& request)
{
	switch (request.type)
	{
	case ECXRequestType::FrustumCheck:
		return handleFrustumCheck(request);
	case ECXRequestType::EntitySearch:
		return handleEntitySearch(request);
	case ECXRequestType::RayCheck:
		return handleRayCheck(request);
	case ECXRequestType::ConeCheck:
		return handleConeCheck(request);
	default:
	{
		ECXResponse response;
		response.response = ECXResponseType::Unsupported;
		return response;
	}
	}
}

ECXResponse EC_BroadPhase::handleFrustumCheck(ECXRequest& request)
{
	ECXResponse response;
	glm::mat4 viewProjection;
	uint32_t layerMask = 0xFFFFFFFFu;

	try {
		viewProjection = std::any_cast<glm::mat4>(request.args[0]);
		if (request.args[1].has_value())
			layerMask = std::any_cast<uint32_t>(request.args[1]);
	}
	catch (const std::bad_any_cast&) {
		response.response = ECXResponseType::Fail;
		return response;
	}

	auto planes = EC_Frustum::extractPlanes(viewProjection);

	// Bounding AABB of the frustum via inverse-transformed NDC cube corners, used to
	// seed the grid cell query before the precise per-candidate plane test.
	glm::mat4 invViewProjection = glm::inverse(viewProjection);
	glm::vec3 boundsMin(std::numeric_limits<float>::max());
	glm::vec3 boundsMax(std::numeric_limits<float>::lowest());
	for (float x : { -1.0f, 1.0f }) {
		for (float y : { -1.0f, 1.0f }) {
			for (float z : { -1.0f, 1.0f }) {
				glm::vec4 corner = invViewProjection * glm::vec4(x, y, z, 1.0f);
				if (corner.w != 0.0f)
					corner /= corner.w;
				boundsMin = glm::min(boundsMin, glm::vec3(corner));
				boundsMax = glm::max(boundsMax, glm::vec3(corner));
			}
		}
	}

	std::vector<EntityID> result;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto candidates = m_SpatialGrid.queryAABB(AABB{ boundsMin, boundsMax });
		result.reserve(candidates.size());
		for (EntityID candidate : candidates) {
			auto it = m_EntityIndex.find(candidate);
			if (it == m_EntityIndex.end())
				continue;
			const EntityAABB& aabb = m_EntityAABBs[it->second];
			if ((aabb.collisionLayer & layerMask) == 0)
				continue;
			if (EC_Frustum::intersectsAABB(planes, aabb.worldAABB))
				result.push_back(candidate);
		}
	}

	response.response = ECXResponseType::Success;
	response.responseData.push_back(result);
	return response;
}

ECXResponse EC_BroadPhase::handleEntitySearch(ECXRequest& request)
{
	ECXResponse response;
	glm::vec3 center;
	float radius = 0.0f;
	uint32_t layerMask = 0xFFFFFFFFu;

	try {
		center = std::any_cast<glm::vec3>(request.args[0]);
		radius = std::any_cast<float>(request.args[1]);
		if (request.args[2].has_value())
			layerMask = std::any_cast<uint32_t>(request.args[2]);
	}
	catch (const std::bad_any_cast&) {
		response.response = ECXResponseType::Fail;
		return response;
	}

	AABB queryRegion{ center - glm::vec3(radius), center + glm::vec3(radius) };
	std::vector<EntityID> result;
	float radiusSq = radius * radius;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto candidates = m_SpatialGrid.queryAABB(queryRegion);
		for (EntityID candidate : candidates) {
			auto it = m_EntityIndex.find(candidate);
			if (it == m_EntityIndex.end())
				continue;
			const EntityAABB& aabb = m_EntityAABBs[it->second];
			if ((aabb.collisionLayer & layerMask) == 0)
				continue;
			glm::vec3 aabbCenter = (aabb.worldAABB.min + aabb.worldAABB.max) * 0.5f;
			glm::vec3 d = aabbCenter - center;
			if (glm::dot(d, d) <= radiusSq)
				result.push_back(candidate);
		}
	}

	response.response = ECXResponseType::Success;
	response.responseData.push_back(result);
	return response;
}

// castsShadow (EC_DOD_GraphicsData::castsShadow) is only ever consulted by ray/cone
// queries, so it's looked up live here rather than cached per-entity in
// broadPhaseCollisionDetection()'s per-tick snapshot - that loop runs on the physics
// thread's unthrottled busy-spin (EC_PhysicsThreadTask::execute()), so paying this cost
// for every collider entity on every tick, regardless of whether anyone is even
// querying, is wasted work on a hot path. Entities with no graphics component (pure
// trigger volumes etc.) default to true so they don't silently disappear from ray
// queries that don't care about shadows.
bool EC_BroadPhase::entityCastsShadow(EntityID entity) const
{
	auto& manager = EC_DOD_EntityManager::getInstance();
	if (!manager.hasComponent<EC_DOD_GraphicsData>(entity))
		return true;
	return manager.getComponent<EC_DOD_GraphicsData>(entity).castsShadow;
}

// Shared broad+precise ray logic (Issue #30) used directly by handleRayCheck, and by
// handleConeCheck for its "unobstructed line-of-sight to apex" occlusion test - this is
// the code-level link satisfying Issue #29's stated dependency on #30.
std::vector<RayQueryHit> EC_BroadPhase::castRay(const glm::vec3& origin, const glm::vec3& dir, float maxDistance,
	uint32_t layerMask, bool requireCastsShadow, bool firstHitOnly)
{
	std::vector<RayQueryHit> hits;

	glm::vec3 end = origin + dir * maxDistance;
	AABB sweptBounds{ glm::min(origin, end), glm::max(origin, end) };

	bool haveBest = false;
	RayQueryHit best;

	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto candidates = m_SpatialGrid.queryAABB(sweptBounds);

		for (EntityID candidate : candidates)
		{
			auto it = m_EntityIndex.find(candidate);
			if (it == m_EntityIndex.end())
				continue;
			const EntityAABB& entry = m_EntityAABBs[it->second];
			if ((entry.collisionLayer & layerMask) == 0)
				continue;
			if (requireCastsShadow && !entityCastsShadow(candidate))
				continue;

			RayIntersectionResult result = EC_RayIntersection::rayVsCollider(origin, dir, entry.collider, entry.spatial);
			if (!result.hit || result.distance > maxDistance)
				continue;

			RayQueryHit hit{ candidate, result.position, result.normal, result.distance };

			if (firstHitOnly)
			{
				if (!haveBest || hit.distance < best.distance)
				{
					best = hit;
					haveBest = true;
				}
			}
			else
			{
				hits.push_back(hit);
			}
		}
	}

	if (firstHitOnly)
	{
		if (haveBest)
			hits.push_back(best);
		return hits;
	}

	std::sort(hits.begin(), hits.end(), [](const RayQueryHit& a, const RayQueryHit& b) {
		return a.distance < b.distance;
		});
	return hits;
}

ECXResponse EC_BroadPhase::handleRayCheck(ECXRequest& request)
{
	ECXResponse response;
	glm::vec3 origin, direction;
	float maxDistance = 0.0f;
	uint32_t layerMask = 0xFFFFFFFFu;
	bool firstHitOnly = false;

	try {
		origin = std::any_cast<glm::vec3>(request.args[0]);
		direction = std::any_cast<glm::vec3>(request.args[1]);
		maxDistance = std::any_cast<float>(request.args[2]);
		if (request.args[3].has_value())
			layerMask = std::any_cast<uint32_t>(request.args[3]);
		if (request.args[4].has_value())
			firstHitOnly = std::any_cast<bool>(request.args[4]);
	}
	catch (const std::bad_any_cast&) {
		response.response = ECXResponseType::Fail;
		return response;
	}

	if (glm::dot(direction, direction) < 1e-8f) {
		response.response = ECXResponseType::Fail;
		return response;
	}
	direction = glm::normalize(direction);

	std::vector<RayQueryHit> hits = castRay(origin, direction, maxDistance, layerMask,
		/*requireCastsShadow*/ false, firstHitOnly);

	response.response = ECXResponseType::Success;
	response.responseData.push_back(hits);
	return response;
}

ECXResponse EC_BroadPhase::handleConeCheck(ECXRequest& request)
{
	ECXResponse response;
	glm::vec3 apex, direction;
	float halfAngleRadians = 0.0f;
	float maxDistance = 0.0f;
	uint32_t layerMask = 0xFFFFFFFFu;
	bool castsShadowOnly = true;
	bool checkOcclusion = false;

	try {
		apex = std::any_cast<glm::vec3>(request.args[0]);
		direction = std::any_cast<glm::vec3>(request.args[1]);
		halfAngleRadians = std::any_cast<float>(request.args[2]);
		maxDistance = std::any_cast<float>(request.args[3]);
		if (request.args[4].has_value())
			layerMask = std::any_cast<uint32_t>(request.args[4]);
		if (request.args[5].has_value())
			castsShadowOnly = std::any_cast<bool>(request.args[5]);
		if (request.args[6].has_value())
			checkOcclusion = std::any_cast<bool>(request.args[6]);
	}
	catch (const std::bad_any_cast&) {
		response.response = ECXResponseType::Fail;
		return response;
	}

	if (glm::dot(direction, direction) < 1e-8f) {
		response.response = ECXResponseType::Fail;
		return response;
	}
	direction = glm::normalize(direction);

	// Broad-phase candidates: bounding box of a sphere of radius maxDistance around the
	// apex (simpler than a tight cone bound, and cheap since the grid query itself is
	// coarse - the exact GJK cone-vs-shape test below does the real filtering).
	AABB queryRegion{ apex - glm::vec3(maxDistance), apex + glm::vec3(maxDistance) };

	// Support function for the cone itself (apex + a finite base disk of radius
	// maxDistance*tan(halfAngle) at distance maxDistance) - the convex hull of a point
	// and a disk. Paired with EC_RayIntersection::colliderSupport() via EC_GJK::intersects,
	// this is an exact geometric cone-vs-shape test (not a bounding-sphere/corner
	// approximation), consistent with the debug wireframe drawn for the same cone.
	// EC_ConvexSupport::coneSupport is the same function EC_GJK_Tests.cpp exercises in
	// isolation.
	EC_GJK::SupportFn coneSupport = [&apex, &direction, halfAngleRadians, maxDistance](const glm::vec3& dir) {
		return EC_ConvexSupport::coneSupport(apex, direction, halfAngleRadians, maxDistance, dir);
	};

	std::vector<std::pair<EntityID, glm::vec3>> candidatesInCone;
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		auto candidates = m_SpatialGrid.queryAABB(queryRegion);
		for (EntityID candidate : candidates)
		{
			auto it = m_EntityIndex.find(candidate);
			if (it == m_EntityIndex.end())
				continue;
			const EntityAABB& entry = m_EntityAABBs[it->second];
			if ((entry.collisionLayer & layerMask) == 0)
				continue;
			if (castsShadowOnly && !entityCastsShadow(candidate))
				continue;
			// Plane/Frustum/None have no bounded support function (colliderSupport()'s
			// degenerate single-point fallback would misrepresent them in a real GJK
			// test) - not valid cone targets, matching rayVsCollider's exclusion.
			if (entry.collider.type == EC_DOD_Collider::Type::Plane ||
				entry.collider.type == EC_DOD_Collider::Type::Frustum ||
				entry.collider.type == EC_DOD_Collider::Type::None)
				continue;

			EC_GJK::SupportFn shapeSupport = [&entry](const glm::vec3& dir) {
				return EC_RayIntersection::colliderSupport(entry.collider, entry.spatial, dir);
			};
			if (!EC_GJK::intersects(coneSupport, shapeSupport))
				continue;

			glm::vec3 candidatePos = entry.spatial.position + entry.collider.center;
			candidatesInCone.emplace_back(candidate, candidatePos);
		}
	}

	// Pure geometric containment by default - every entity whose shape overlaps the
	// cone. checkOcclusion opts into additionally requiring unobstructed line-of-sight
	// to the apex (a candidate stacked behind a closer one along the same line is
	// excluded) - an independent, optional layer on top of containment, not fused into
	// it, since callers may want either.
	std::vector<RayQueryHit> results;
	results.reserve(candidatesInCone.size());
	for (const auto& [candidateEntity, candidatePos] : candidatesInCone)
	{
		float dist = glm::length(candidatePos - apex);

		if (checkOcclusion)
		{
			glm::vec3 rayDir = (candidatePos - apex) / dist;
			std::vector<RayQueryHit> occlusionHits = castRay(apex, rayDir, dist, layerMask,
				castsShadowOnly, /*firstHitOnly*/ true);
			if (occlusionHits.empty() || occlusionHits[0].entity != candidateEntity)
				continue; // nothing hit, or something closer blocks line of sight
		}

		results.push_back(RayQueryHit{ candidateEntity, candidatePos, glm::vec3(0.0f), dist });
	}

	response.response = ECXResponseType::Success;
	response.responseData.push_back(results);
	return response;
}

// Helper function to compute world-space AABB from collider and spatial
AABB EC_BroadPhase::computeWorldAABB(const EC_DOD_Collider& collider,
    const EC_DOD_Spatial& spatial)
{
    AABB worldAABB;
    glm::vec3 worldCenter = spatial.position + collider.center;

    // Compute orientation matrix from direction and up vectors
    glm::vec3 forward = glm::normalize(spatial.direction);
    glm::vec3 right = glm::normalize(spatial.right);
    glm::vec3 up = glm::normalize(spatial.up);

    switch (collider.type) {
    case EC_DOD_Collider::Type::Sphere: {
        // Sphere AABB is simple - just expand in all directions by radius
        worldAABB.min = worldCenter - glm::vec3(collider.radius);
        worldAABB.max = worldCenter + glm::vec3(collider.radius);
        break;
    }

    case EC_DOD_Collider::Type::AABB: {
        // AABB in world space (axis-aligned, no rotation)
        worldAABB.min = worldCenter - collider.extents;
        worldAABB.max = worldCenter + collider.extents;
        break;
    }

    case EC_DOD_Collider::Type::OBB: {
        // OBB needs to transform corners to world space
        glm::vec3 corners[8];
        corners[0] = glm::vec3(-collider.extents.x, -collider.extents.y, -collider.extents.z);
        corners[1] = glm::vec3(collider.extents.x, -collider.extents.y, -collider.extents.z);
        corners[2] = glm::vec3(-collider.extents.x, collider.extents.y, -collider.extents.z);
        corners[3] = glm::vec3(collider.extents.x, collider.extents.y, -collider.extents.z);
        corners[4] = glm::vec3(-collider.extents.x, -collider.extents.y, collider.extents.z);
        corners[5] = glm::vec3(collider.extents.x, -collider.extents.y, collider.extents.z);
        corners[6] = glm::vec3(-collider.extents.x, collider.extents.y, collider.extents.z);
        corners[7] = glm::vec3(collider.extents.x, collider.extents.y, collider.extents.z);

        // Build rotation matrix from spatial orientation
        glm::mat3 rotation = glm::mat3(right, up, forward);

        // Transform first corner
        glm::vec3 worldCorner = worldCenter + rotation * corners[0];
        worldAABB.min = worldCorner;
        worldAABB.max = worldCorner;

        // Transform remaining corners and expand AABB
        for (int i = 1; i < 8; ++i) {
            worldCorner = worldCenter + rotation * corners[i];
            worldAABB.min = glm::min(worldAABB.min, worldCorner);
            worldAABB.max = glm::max(worldAABB.max, worldCorner);
        }
        break;
    }

    case EC_DOD_Collider::Type::Capsule: {
        // Capsule AABB includes the line segment + radius
        glm::vec3 halfHeight = up * (collider.height * 0.5f);
        glm::vec3 pointA = worldCenter - halfHeight;
        glm::vec3 pointB = worldCenter + halfHeight;

        worldAABB.min = glm::min(pointA, pointB) - glm::vec3(collider.radius);
        worldAABB.max = glm::max(pointA, pointB) + glm::vec3(collider.radius);
        break;
    }

    case EC_DOD_Collider::Type::Cylinder: {
        // Cylinder AABB approximation
        glm::vec3 halfHeight = up * (collider.height * 0.5f);

        // Calculate the maximum extent in the radial directions
        glm::vec3 radialExtent = glm::vec3(collider.radius);

        worldAABB.min = worldCenter - halfHeight - radialExtent;
        worldAABB.max = worldCenter + halfHeight + radialExtent;
        break;
    }

    case EC_DOD_Collider::Type::Frustum: {
        // Frustum AABB is complex - for now use a conservative box
        // This would need proper frustum corner calculation for accuracy
        float maxExtent = glm::max(collider.radius, collider.height);
        worldAABB.min = worldCenter - glm::vec3(maxExtent);
        worldAABB.max = worldCenter + glm::vec3(maxExtent);
        break;
    }

    case EC_DOD_Collider::Type::Plane: {
        // Planes are infinite - use a very large AABB
        const float planeSize = 10000.0f;
        worldAABB.min = worldCenter - glm::vec3(planeSize);
        worldAABB.max = worldCenter + glm::vec3(planeSize);
        break;
    }

    default:
    case EC_DOD_Collider::Type::None: {
        worldAABB.min = worldCenter;
        worldAABB.max = worldCenter;
        break;
    }
    }

    return worldAABB;
}