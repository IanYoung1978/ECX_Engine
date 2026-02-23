#pragma once
#include <glm/glm.hpp>
#include "EC_CollisionShapes.h"
#include <vector>

namespace EC_CollisionChecks
{

	bool SphereVsSphere(const Sphere& sphereA, const glm::vec3& posA,
		const Sphere& sphereB, const glm::vec3& posB,
		CollisionManifold& manifold);
	bool AABBVsAABB(const AABB& aabbA, const glm::vec3& posA,
		const AABB& aabbB, const glm::vec3& posB,
		CollisionManifold& manifold);
	bool OBBVsOBB(const OBB& obbA, const glm::vec3& posA,
		const OBB& obbB, const glm::vec3& posB,
		CollisionManifold& manifold);
	bool SphereVsAABB(const Sphere& sphere, const glm::vec3& spherePos,
		const AABB& aabb, const glm::vec3& aabbPos,
		CollisionManifold& manifold);
	bool SphereVsOBB(const Sphere& sphere, const glm::vec3& spherePos,
		const OBB& obb, const glm::vec3& obbPos,
		CollisionManifold& manifold);
	bool FrustumVsAABB(const Frustum& frustum, const glm::vec3& frustumPos,
		const AABB& aabb, const glm::vec3& aabbPos);


}
