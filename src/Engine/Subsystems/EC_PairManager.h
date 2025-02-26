#pragma once
#include <vector>
#include <glm\glm.hpp>
#include <memory>
struct EC_CollisionPair
{
	unsigned int body_A;
	unsigned int body_B;
	bool m_Colliding;
	std::vector<glm::vec3> m_CollisionPoints;
	EC_CollisionPair() :body_A(0), body_B(0), m_Colliding(false) {}
};
class EC_PairManager
{
public:
	EC_PairManager();
	~EC_PairManager();
	void addPair(unsigned int id_a, unsigned int id_b);
	void update();
	bool hasPairs();
	EC_CollisionPair& getNextPair();
private:
	static std::vector<EC_CollisionPair> s_Pairs;
	static std::vector<EC_CollisionPair>::iterator s_CurrentPair;
};

