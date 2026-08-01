#include "EC_PairManager.h"
#include <algorithm>
#include <stdexcept>

std::vector<EC_CollisionPair> EC_PairManager::s_Pairs;
size_t EC_PairManager::s_CurrentPairIndex = 0;

EC_PairManager::EC_PairManager()
{
}


EC_PairManager::~EC_PairManager()
{
}

void EC_PairManager::addPair(uint32_t id_a, uint32_t id_b)
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
	// Remove all non-colliding pairs and keep only actively colliding pairs.
	auto endIt = std::remove_if(
		s_Pairs.begin(),
		s_Pairs.end(),
		[](const EC_CollisionPair& pair)
		{
			return !pair.m_Colliding;
		});

	s_Pairs.erase(endIt, s_Pairs.end());

	// Reset traversal cursor for next frame.
	s_CurrentPairIndex = 0;

}

bool EC_PairManager::hasPairs()
{
	return s_CurrentPairIndex < s_Pairs.size();
}


EC_CollisionPair& EC_PairManager::getNextPair()
{
	if (!hasPairs())
	{
		throw std::out_of_range("No collision pairs available");
	}

	return s_Pairs[s_CurrentPairIndex++];
}

std::vector<EC_CollisionPair>& EC_PairManager::getAllPairs()
{
	return s_Pairs;
}

