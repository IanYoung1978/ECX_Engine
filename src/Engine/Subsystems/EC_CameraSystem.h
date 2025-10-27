#pragma once
#include "EC_System.h"
#include "Common/EC_ProxyObserver.h"
#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>
#include <vector>
#include "Messaging/ECXMessenger.h"

class EC_CameraComponent;
class Spatial;
class EC_Entity;


class EC_CameraSystem :
	public EC_System
{
public:
	EC_CameraSystem();
	virtual ~EC_CameraSystem();

	// Inherited via EC_System
	virtual void init(ECXMessenger& messenger, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game & game) override;
};

