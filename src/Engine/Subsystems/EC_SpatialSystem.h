#pragma once
#include "EC_System.h"
#include "Entity/GameEntity.h"
#include "Common/EC_ProxyObserver.h"
#include <glm\glm.hpp>

class Transform;
class Spatial;
class EC_EventQueue;

class EC_SpatialSystem :
	public EC_System
{
public:
	EC_SpatialSystem();
	virtual ~EC_SpatialSystem();

	// Inherited via EC_System
	virtual void init(ECXMessenger& queue, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game& game) override;
private:
};

