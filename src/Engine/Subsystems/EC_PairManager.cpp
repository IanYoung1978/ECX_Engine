#include "EC_PairManager.h"
#include <algorithm>

std::vector<EC_CollisionPair> EC_PairManager::s_Pairs;
std::vector<EC_CollisionPair>::iterator EC_PairManager::s_CurrentPair;

EC_PairManager::EC_PairManager()
{
}


EC_PairManager::~EC_PairManager()
{
}

void EC_PairManager::addPair(unsigned int id_a, unsigned int id_b)
{
	auto pred = [&](EC_CollisionPair& pair)
	{ 
		return ((pair.body_A == id_a) && (pair.body_B == id_b)) || ((pair.body_B == id_a) && (pair.body_A == id_b));
	};

	auto pair = std::find_if(s_Pairs.begin(), s_Pairs.end(), pred);

	if (pair == s_Pairs.end())
	{
		s_Pairs.emplace_back();
		s_Pairs.back().body_A = id_a;
		s_Pairs.back().body_B = id_b;
		s_Pairs.back().m_Colliding = false;
	}
}

void EC_PairManager::update()
{
	// Updating the stored pairs is simple:
	// All pairs that are not colliding are pushed to the back of the vector
	auto sort = [](const EC_CollisionPair& pair, const EC_CollisionPair& other) { return pair.m_Colliding == false; };
	std::sort(s_Pairs.begin(), s_Pairs.end(), sort);
	size_t num_Valid = 0;
	//then we count up the number of valid colliding pairs
	for (auto & pair : s_Pairs)
	{
		if (pair.m_Colliding)
			num_Valid++;
		else
			break;
	}
	//and resize the vector to that number. Obselete pairs are destroyed
	s_Pairs.resize(num_Valid);
	//then reset the iterator for the next frame
	s_CurrentPair = s_Pairs.begin();
}

bool EC_PairManager::hasPairs()
{
	return (s_CurrentPair != s_Pairs.end());
}

EC_CollisionPair& EC_PairManager::getNextPair()
{
	//dereference the iterator,
	//return reference
	//increment iterator
	//operator precedence trick.
	return *s_CurrentPair++;
}
