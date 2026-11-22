#pragma once
#include <glm/glm.hpp>
#include "EC_CollisionShapes.h"
#include <vector>
#include <cstdint>

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

	// Capsule.pointA/pointB are local (add the separate pos for world space), like the other shapes.
	// Each Capsule function's contactNormal points toward its second parameter, away from the capsule;
	// EC_NarrowPhase's dispatch table flips it where that parameter lands in slot A.
	bool CapsuleVsSphere(const Capsule& capsule, const glm::vec3& capsulePos,
		const Sphere& sphere, const glm::vec3& spherePos,
		CollisionManifold& manifold);
	bool CapsuleVsAABB(const Capsule& capsule, const glm::vec3& capsulePos,
		const AABB& aabb, const glm::vec3& aabbPos,
		CollisionManifold& manifold);
	bool CapsuleVsOBB(const Capsule& capsule, const glm::vec3& capsulePos,
		const OBB& obb, const glm::vec3& obbPos,
		CollisionManifold& manifold);
	// Self-orienting (like SphereVsSphere) - contactNormal already points capsuleA ->
	// capsuleB, no flip needed regardless of dispatch ordering.
	bool CapsuleVsCapsule(const Capsule& capsuleA, const glm::vec3& posA,
		const Capsule& capsuleB, const glm::vec3& posB,
		CollisionManifold& manifold);

	// Capsule vs triangle mesh, testing every triangle (with a bounding-box cull). Takes plain vectors
	// so this file stays free of ECS dependencies; positions/normals are mesh-local, meshPos moves them
	// to world space. contactNormal is the winning triangle's averaged per-vertex gradient normal, not a
	// cross product: the mesher's winding can flip on some triangles, the gradient normals can't.
	bool CapsuleVsMesh(const Capsule& capsule, const glm::vec3& capsulePos,
		const std::vector<glm::vec3>& meshPositions, const std::vector<glm::vec3>& meshNormals,
		const std::vector<uint32_t>& meshIndices,
		const glm::vec3& meshPos,
		CollisionManifold& manifold);

}
