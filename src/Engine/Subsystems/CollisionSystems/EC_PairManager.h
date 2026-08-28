#pragma once
#include <vector>
#include <glm\glm.hpp>
#include <memory>
#include <cstddef>

// One contact point's accumulated impulse, persisted across ticks while the
// pair keeps colliding - "warm starting". Physics resolution seeds each
// tick's solve from last tick's converged total instead of starting at zero
// every time, which is what lets a persistent contact (e.g. a box resting
// or tumbling on the floor across many consecutive ticks) actually converge
// tick-to-tick instead of re-deriving the same answer from scratch and
// re-introducing solver noise every frame - standard technique in every
// production physics engine (Box2D, Bullet, etc).
struct EC_ContactImpulseCache
{
	glm::vec3 point{ 0.0f };
	float normalImpulse = 0.0f;
	float tangentImpulse = 0.0f;
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
	// Warm-start cache, one entry per contact point, matched frame-to-frame
	// by closest position (see EC_PhysicsResolution::accumulateImpulses) -
	// owned and updated entirely by the physics system. Naturally cleared
	// when the pair itself is destroyed (EC_PairManager::update() erases
	// non-colliding pairs), so no separate cleanup is needed when contact
	// genuinely ends.
	std::vector<EC_ContactImpulseCache> m_ContactCache;
	EC_CollisionPair() :body_A(0), body_B(0), m_Colliding(false) {}
};
// All state below is physics-thread-only working data - collision pairs are
// produced and consumed entirely within one tick of the collision/physics
// pipeline (EC_BroadPhase -> EC_NarrowPhase -> EC_CollisionSystem ->
// EC_PhysicsSystem), all of which run sequentially on the same thread. It is
// NOT safe to read from any other thread (e.g. the render thread) - there is
// no synchronization, by design, because nothing outside this pipeline is
// meant to touch it. Debug rendering gets contact-point data via
// EC_DOD_DebugContacts instead (published once per tick as component data by
// EC_PhysicsSystem) rather than reading this class directly - see
// GL_DebugRenderer::renderContactPoints. The methods below are private and
// friended to exactly the classes that make up that pipeline, to keep it that
// way rather than relying on convention.
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
	// Mutable view of every tracked pair, independent of the hasPairs()/
	// getNextPair() consuming cursor - lets the physics system iterate all
	// currently-colliding pairs after collision detection has already
	// drained the cursor this tick. Mutable (not just read-only) so physics
	// can update each pair's m_ContactCache for next tick's warm start.
	static std::vector<EC_CollisionPair>& getAllPairs();

	static std::vector<EC_CollisionPair> s_Pairs;
	static size_t s_CurrentPairIndex;
};

