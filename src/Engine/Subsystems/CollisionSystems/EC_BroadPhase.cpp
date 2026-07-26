#include "EC_BroadPhase.h"
#include "Messaging/ECXMessenger.h"
#include "Messaging/ECXRequest.h"
#include "Messaging/ECXResponse.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "EC_CollisionShapes.h"
#include "Spatial/EC_Frustum.h"
#include <limits>

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
                localAABBs.push_back({ entityId, worldAABB, collider.collisionLayer, collider.collisionMask });
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
}

ECXResponse EC_BroadPhase::receive(ECXRequest& request)
{
	switch (request.type)
	{
	case ECXRequestType::FrustumCheck:
		return handleFrustumCheck(request);
	case ECXRequestType::EntitySearch:
		return handleEntitySearch(request);
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