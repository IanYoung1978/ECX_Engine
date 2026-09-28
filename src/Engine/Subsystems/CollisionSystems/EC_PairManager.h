#pragma once
#include <vector>
#include <glm\glm.hpp>
#include <memory>
#include <cstddef>

// One contact point's accumulated impulse, kept across ticks while the pair keeps colliding (warm
// starting): each solve starts from last tick's converged total rather than zero, so persistent
// contacts converge instead of re-deriving the answer and reintroducing noise every frame.
struct EC_ContactImpulseCache
{
	glm::vec3 point{ 0.0f };
	float normalImpulse = 0.0f;
	// Friction impulse in the contact's tangent plane (2 components - the
	// full Coulomb friction disc, not a single sliding-direction scalar).
	glm::vec2 tangentImpulse{ 0.0f };
};

struct EC_CollisionPair
{
	uint32_t body_A;
	uint32_t body_B;
	bool m_Colliding;
	std::vector<glm::vec3> m_CollisionPoints;
	// Geometric manifold data cached by collision detection (EC_NarrowPhase)
	// for the physics system to consume later this tick - collision
	// resolution only ever writes these, it never computes forces/impulses
	// from them itself.
	glm::vec3 m_ContactNormal{ 0.0f };
	float m_PenetrationDepth = 0.0f;
	// Warm-start cache, one entry per contact point, matched frame to frame by closest position (see
	// EC_PhysicsResolution::accumulateImpulses). Owned by the physics system; cleared with the pair when
	// EC_PairManager::update() erases it.
	std::vector<EC_ContactImpulseCache> m_ContactCache;
	// Physics-thread-only. EC_NarrowPhase compares these against a Kinematic body's current Spatial;
	// if either side moved while the pair was already colliding, that tick is treated as a fresh BEGIN
	// so scripts get updated contact data. Dynamic bodies don't need it: they are re-resolved every tick.
	glm::vec3 m_LastCheckedPositionA{ 0.0f };
	glm::vec3 m_LastCheckedOrientationA{ 0.0f };
	glm::vec3 m_LastCheckedPositionB{ 0.0f };
	glm::vec3 m_LastCheckedOrientationB{ 0.0f };
	EC_CollisionPair() :body_A(0), body_B(0), m_Colliding(false) {}
};
// Physics-thread-only working data, no synchronization by design. The methods below are private and
// friended to the collision/physics pipeline classes. Debug rendering reads EC_DOD_DebugContacts.
class EC_BroadPhase;
class EC_NarrowPhase;
class EC_CollisionSystem;
class EC_PhysicsSystem;
// Test-only accessor (see tests/EC_PairManager_Tests.cpp) - lets the Catch2
// suite exercise addPair/update/hasPairs/getNextPair/getAllPairs directly
// without opening those up to the rest of the engine.
class EC_PairManagerTestHelper;

class EC_PairManager
{
public:
	EC_PairManager();
	~EC_PairManager();

private:
	friend class EC_BroadPhase;
	friend class EC_NarrowPhase;
	friend class EC_CollisionSystem;
	friend class EC_PhysicsSystem;
	friend class EC_PairManagerTestHelper;

	void addPair(uint32_t id_a, uint32_t id_b);
	void update();
	bool hasPairs();
	EC_CollisionPair& getNextPair();
	// Mutable view of every tracked pair, independent of the consuming hasPairs()/getNextPair() cursor,
	// so physics can iterate all colliding pairs after detection drained it and update each
	// m_ContactCache for the next tick's warm start.
	static std::vector<EC_CollisionPair>& getAllPairs();

	static std::vector<EC_CollisionPair> s_Pairs;
	static size_t s_CurrentPairIndex;
};

