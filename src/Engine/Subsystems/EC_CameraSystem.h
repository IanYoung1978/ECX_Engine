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


class EC_CameraSystemProxy :
	public EC_ProxyObserver
{
public:
	EC_CameraSystemProxy();
	EC_CameraSystemProxy(std::shared_ptr<GameEntity>& e);
	~EC_CameraSystemProxy();
	EC_CameraSystemProxy(EC_CameraSystemProxy&& proxy) noexcept;
	EC_CameraSystemProxy(EC_CameraSystemProxy& proxy);
	EC_CameraSystemProxy& operator= (EC_CameraSystemProxy& proxy);
	void Register();
	void UnRegister();
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;

	std::shared_ptr<GameEntity> owner;
	std::shared_ptr<EC_CameraComponent> cam;
	std::shared_ptr<Spatial> spatial;
	glm::vec3 position;
	glm::vec3 orientation;
	float fov,
		draw_distance;
	int view_w,
		view_h;
	bool isActive;
};

class EC_CameraSystem :
	public EC_System
{
public:
	EC_CameraSystem();
	virtual ~EC_CameraSystem();

	// Inherited via EC_System
	virtual void init(ECXMessenger& messenger, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game & game) override;
	virtual void addEntity(std::shared_ptr<GameEntity>& e) override;
	virtual void removeEntity(unsigned int entityID) override;
	virtual void clearEntities() override;

private:
	std::vector<EC_CameraSystemProxy> m_entities;
};

