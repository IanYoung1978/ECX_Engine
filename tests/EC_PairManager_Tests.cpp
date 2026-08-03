// Unit tests for EC_PairManager. Note: all of its state is static/class-level
// (shared across every instance, and across every test case in this binary),
// with no reset() - so every TEST_CASE here clears EC_PairManager::getAllPairs()
// itself on entry to stay isolated from whatever ran before it.
#include <catch2/catch_test_macros.hpp>
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"

TEST_CASE("EC_PairManager tracks unique pairs regardless of argument order", "[PairManager]") {
    EC_PairManager::getAllPairs().clear();
    EC_PairManager mgr;

    mgr.addPair(1, 2);
    mgr.addPair(2, 1); // same pair, reversed order - must not duplicate
    mgr.addPair(3, 4);

    REQUIRE(EC_PairManager::getAllPairs().size() == 2);
}

TEST_CASE("EC_PairManager::update() purges pairs that never went colliding", "[PairManager]") {
    EC_PairManager::getAllPairs().clear();
    EC_PairManager mgr;

    mgr.addPair(1, 2);
    mgr.addPair(3, 4);
    EC_PairManager::getAllPairs()[0].m_Colliding = true; // pair(1,2) stays
    // pair(3,4) left non-colliding -> should be dropped by update()

    mgr.update();

    REQUIRE(EC_PairManager::getAllPairs().size() == 1);
    REQUIRE(EC_PairManager::getAllPairs()[0].body_A == 1);
    REQUIRE(EC_PairManager::getAllPairs()[0].body_B == 2);
}

TEST_CASE("EC_PairManager's consuming cursor (hasPairs/getNextPair) drains once per update() cycle", "[PairManager]") {
    EC_PairManager::getAllPairs().clear();
    EC_PairManager mgr;

    mgr.addPair(1, 2);
    mgr.addPair(3, 4);
    for (auto& p : EC_PairManager::getAllPairs()) p.m_Colliding = true;
    mgr.update(); // resets the cursor to 0

    REQUIRE(mgr.hasPairs());
    mgr.getNextPair();
    REQUIRE(mgr.hasPairs());
    mgr.getNextPair();
    REQUIRE_FALSE(mgr.hasPairs());

    REQUIRE_THROWS_AS(mgr.getNextPair(), std::out_of_range);

    EC_PairManager::getAllPairs().clear();
}
