// Unit tests for EC_PairManager. Note: all of its state is static/class-level
// (shared across every instance, and across every test case in this binary),
// with no reset() - so every TEST_CASE here clears EC_PairManager::getAllPairs()
// itself on entry to stay isolated from whatever ran before it.
//
// EC_PairManager's methods are private, friended only to the physics-thread
// pipeline classes (EC_BroadPhase/EC_NarrowPhase/EC_CollisionSystem/
// EC_PhysicsSystem) plus this helper - deliberately, so nothing else in the
// engine (e.g. render-thread code) can read physics-thread-internal pair
// state directly. EC_PairManagerTestHelper is declared a friend in
// EC_PairManager.h purely so this suite can still exercise it directly.
#include <catch2/catch_test_macros.hpp>
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"

class EC_PairManagerTestHelper {
public:
    static void addPair(EC_PairManager& mgr, uint32_t a, uint32_t b) { mgr.addPair(a, b); }
    static void update(EC_PairManager& mgr) { mgr.update(); }
    static bool hasPairs(EC_PairManager& mgr) { return mgr.hasPairs(); }
    static EC_CollisionPair& getNextPair(EC_PairManager& mgr) { return mgr.getNextPair(); }
    static std::vector<EC_CollisionPair>& getAllPairs() { return EC_PairManager::getAllPairs(); }
};

namespace {
    using Helper = EC_PairManagerTestHelper;
}

TEST_CASE("EC_PairManager tracks unique pairs regardless of argument order", "[PairManager]") {
    Helper::getAllPairs().clear();
    EC_PairManager mgr;

    Helper::addPair(mgr, 1, 2);
    Helper::addPair(mgr, 2, 1); // same pair, reversed order - must not duplicate
    Helper::addPair(mgr, 3, 4);

    REQUIRE(Helper::getAllPairs().size() == 2);
}

TEST_CASE("EC_PairManager::update() purges pairs that never went colliding", "[PairManager]") {
    Helper::getAllPairs().clear();
    EC_PairManager mgr;

    Helper::addPair(mgr, 1, 2);
    Helper::addPair(mgr, 3, 4);
    Helper::getAllPairs()[0].m_Colliding = true; // pair(1,2) stays
    // pair(3,4) left non-colliding -> should be dropped by update()

    Helper::update(mgr);

    REQUIRE(Helper::getAllPairs().size() == 1);
    REQUIRE(Helper::getAllPairs()[0].body_A == 1);
    REQUIRE(Helper::getAllPairs()[0].body_B == 2);
}

TEST_CASE("EC_PairManager's consuming cursor (hasPairs/getNextPair) drains once per update() cycle", "[PairManager]") {
    Helper::getAllPairs().clear();
    EC_PairManager mgr;

    Helper::addPair(mgr, 1, 2);
    Helper::addPair(mgr, 3, 4);
    for (auto& p : Helper::getAllPairs()) p.m_Colliding = true;
    Helper::update(mgr); // resets the cursor to 0

    REQUIRE(Helper::hasPairs(mgr));
    Helper::getNextPair(mgr);
    REQUIRE(Helper::hasPairs(mgr));
    Helper::getNextPair(mgr);
    REQUIRE_FALSE(Helper::hasPairs(mgr));

    REQUIRE_THROWS_AS(Helper::getNextPair(mgr), std::out_of_range);

    Helper::getAllPairs().clear();
}
