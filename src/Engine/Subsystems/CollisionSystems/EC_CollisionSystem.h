#pragma once

#include "Engine/Subsystems/EC_System.h"

#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"
#include "EC_BroadPhase.h"
#include "EC_NarrowPhase.h"

class EC_Game;

class EC_CollisionSystem
	: public EC_System
{
	public:
	EC_CollisionSystem();
	~EC_CollisionSystem();
	// Inherited via EC_System
	virtual void init(ECXMessenger& messenger, EC_Game& game) override;
	virtual void update(const float& deltaTimeS, EC_Game& game) override;

	// Exposes the same pair manager broad/narrow phase populate, so
	// EC_PhysicsSystem can be wired to it once at startup and iterate the
	// cached collision manifolds after this system's update() has run.
	EC_PairManager& getPairManager() { return m_PairManager; }
private:
	std::unique_ptr<EC_BroadPhase> m_BroadPhase;
	std::unique_ptr<EC_NarrowPhase> m_NarrowPhase;

	ECXMessenger* m_Messenger;
	EC_PairManager m_PairManager;
};