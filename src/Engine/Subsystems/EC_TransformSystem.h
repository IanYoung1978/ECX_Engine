#pragma once
#include "EC_System.h"
#include "Common/EC_ProxyObserver.h"
#include "Entity/GameEntity.h"
#include <memory>
#include <glm\glm.hpp>
#include "Components/Transform.h"
#include "Components/Spatial.h"
#include <vector>
#include "Messaging/ECXMessenger.h"

class EC_EventQueue;


class EC_TransformSystem :
	public EC_System
{
public:
	EC_TransformSystem();
	virtual ~EC_TransformSystem();

	// Inherited via EC_System
	virtual void init(ECXMessenger& messenger, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game & game) override;
private:
};

