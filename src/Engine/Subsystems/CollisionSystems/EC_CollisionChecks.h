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

	// Capsule.pointA/pointB are local, same convention as Sphere.center/AABB.min-max/
	// OBB.center - add the separate pos parameter to get world-space endpoints. Each
	// function's contactNormal points toward its second parameter (sphere/aabb/obb),
	// away from the capsule - EC_NarrowPhase's dispatch table flips it on whichever
	// ordering puts that second parameter in dispatch-slot A, same as it already does
	// for SphereVsAABB/SphereVsOBB's own "points toward the sphere" convention.
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

	// Real capsule-vs-triangle-mesh collision detection (not a raycast substitute) -
	// brute-force closest point between the capsule's segment and every triangle in
	// meshPositions/meshIndices, same computational pattern as EC_RayIntersection::
	// rayMesh's per-triangle iteration but testing closest-approach distance instead of
	// ray crossing. Deliberately takes plain vectors rather than EC_DOD_MeshCollisionData
	// directly, so this file (and its pure-C++ unit tests) stay free of any ECS/engine
	// dependency - callers with a live EC_DOD_MeshCollisionData (e.g. EC_BroadPhase) just
	// dereference its shared_ptr<vector>s. meshPositions/meshNormals are in the mesh's own
	// local space; meshPos is added to get world space, matching EC_DOD_MeshCollisionData's
	// chunk-local convention.
	// contactNormal is the winning triangle's own per-vertex gradient normal (averaged over
	// its 3 vertices, from meshNormals - NOT derived from the triangle's vertex winding via
	// a cross product). EC_MarchingCubesMesher::emitTriangle's winding fix-up has a known
	// parity bug at some cube-decomposition boundaries that can flip a triangle's winding
	// relative to its neighbours even on a perfectly flat, continuous surface - a
	// cross-product-based normal inherits that flip, but the gradient normals in
	// meshNormals are computed straight from the density field and don't depend on winding
	// at all, so they stay reliable exactly where cross-product normals aren't.
	bool CapsuleVsMesh(const Capsule& capsule, const glm::vec3& capsulePos,
		const std::vector<glm::vec3>& meshPositions, const std::vector<glm::vec3>& meshNormals,
		const std::vector<uint32_t>& meshIndices,
		const glm::vec3& meshPos,
		CollisionManifold& manifold);

}
