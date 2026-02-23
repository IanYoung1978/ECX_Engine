#pragma once
#include <vector>
#include <glm\glm.hpp>
#include <memory>
#include <cstddef>

struct EC_CollisionPair
{
	uint32_t body_A;
	uint32_t body_B;
	bool m_Colliding;
	std::vector<glm::vec3> m_CollisionPoints;
	EC_CollisionPair() :body_A(0), body_B(0), m_Colliding(false) {}
};
class EC_PairManager
{
public:
	EC_PairManager();
	~EC_PairManager();
	void addPair(uint32_t id_a, uint32_t id_b);
	void update();
	bool hasPairs();
	EC_CollisionPair& getNextPair();
private:
	static std::vector<EC_CollisionPair> s_Pairs;
	static size_t s_CurrentPairIndex;
};

