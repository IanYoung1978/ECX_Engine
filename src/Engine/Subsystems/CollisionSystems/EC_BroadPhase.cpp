#include "EC_BroadPhase.h"
#include "Messaging/ECXMessenger.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "EC_CollisionShapes.h"

void EC_BroadPhase::broadPhaseCollisionDetection()
{
    // Get all entities with collider and spatial components
    auto entities = EC_DOD_EntityManager::getInstance().getEntitiesWithComponents({
        std::type_index(typeid(EC_DOD_Collider)),
        std::type_index(typeid(EC_DOD_Spatial))
        });

    if (entities.empty())
        return;

    // Get component arrays
    auto* colliderArray = EC_DOD_EntityManager::getInstance().getComponentArray<EC_DOD_Collider>();
    auto* spatialArray = EC_DOD_EntityManager::getInstance().getComponentArray<EC_DOD_Spatial>();

    if (!colliderArray || !spatialArray)
        return;

    // Compute world-space AABBs for each entity
    struct EntityAABB {
        uint32_t entityId;
        AABB worldAABB;
        uint32_t collisionLayer;
        uint32_t collisionMask;
    };

    std::vector<EntityAABB> entityAABBs;
    entityAABBs.reserve(entities.size());

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
        entityAABBs.push_back({ entityId, worldAABB, collider.collisionLayer, collider.collisionMask });
    }

    // Broad-phase collision detection using Sweep and Prune (Sort and Sweep)
    // This is efficient for many objects and handles temporal coherence well

    // Sort entities by minimum X coordinate
    std::sort(entityAABBs.begin(), entityAABBs.end(),
        [](const EntityAABB& a, const EntityAABB& b) {
            return a.worldAABB.min.x < b.worldAABB.min.x;
        });

    // Sweep and prune algorithm
    for (size_t i = 0; i < entityAABBs.size(); ++i) {
        const EntityAABB& entityA = entityAABBs[i];

        // Check against all entities whose min.x is less than entityA's max.x
        for (size_t j = i + 1; j < entityAABBs.size(); ++j) {
            const EntityAABB& entityB = entityAABBs[j];

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



void EC_BroadPhase::init(ECXMessenger& messenger)
{
	messenger.Subscribe(*this, ECXRequestType::FrustumCheck);
}

ECXResponse& EC_BroadPhase::receive(ECXRequest& request)
{
	// TODO: insert return statement here
	// handle frustum check request
	ECXResponse response;
	// perform frustum check broad-phase collision detection
	// cheap AABB vs Frustum checks
	// populate response with results

	response.response = ECXResponseType::Unsupported;
	
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