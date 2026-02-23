#include "EC_CollisionChecks.h"

bool EC_CollisionChecks::SphereVsSphere(const Sphere& sphereA, const glm::vec3& posA, const Sphere& sphereB, const glm::vec3& posB, CollisionManifold& manifold)
{
	return false;
}

bool EC_CollisionChecks::AABBVsAABB(const AABB& aabbA, const glm::vec3& posA, const AABB& aabbB, const glm::vec3& posB, CollisionManifold& manifold)
{
	return false;
}

bool EC_CollisionChecks::OBBVsOBB(const OBB& obbA, const glm::vec3& posA, const OBB& obbB, const glm::vec3& posB, CollisionManifold& manifold)
{
	return false;
}

bool EC_CollisionChecks::SphereVsAABB(const Sphere& sphere, const glm::vec3& spherePos, const AABB& aabb, const glm::vec3& aabbPos, CollisionManifold& manifold)
{
	return false;
}

bool EC_CollisionChecks::SphereVsOBB(const Sphere& sphere, const glm::vec3& spherePos, const OBB& obb, const glm::vec3& obbPos, CollisionManifold& manifold)
{
	return false;
}

bool EC_CollisionChecks::FrustumVsAABB(const Frustum& frustum, const glm::vec3& frustumPos, const AABB& aabb, const glm::vec3& aabbPos)
{
	return false;
}
