#pragma once

#include "Messaging/IRequestResponder.h"
#include "Engine/Subsystems/CollisionSystems/EC_PairManager.h"
#include "Components/EC_DOD_Components.h"
#include "EC_CollisionShapes.h"

class EC_Game;
class ECXMessenger;

class EC_BroadPhase:
	public IRequestResponder
{
	public:
	EC_BroadPhase() = default;
	virtual ~EC_BroadPhase() = default;
	virtual void broadPhaseCollisionDetection();
	void init(ECXMessenger& messenger);
	// Inherited via IRequestResponder
	virtual ECXResponse& receive(ECXRequest& request) override;
	
private:
	AABB computeWorldAABB(const EC_DOD_Collider& collider, const EC_DOD_Spatial& spatial);
	EC_PairManager m_PairManager;
};