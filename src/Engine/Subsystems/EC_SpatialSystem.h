#pragma once
#include "EC_System.h"
#include "Entity/GameEntity.h"
#include "Common/EC_ProxyObserver.h"
#include <glm\glm.hpp>

class Transform;
class Spatial;
class EC_EventQueue;

class EC_SpatialProxy 
	:public EC_ProxyObserver
{
public:
	EC_SpatialProxy() { ; }
	EC_SpatialProxy(std::shared_ptr<GameEntity> entity);
	~EC_SpatialProxy();
	EC_SpatialProxy(EC_SpatialProxy&& proxy) noexcept;
	EC_SpatialProxy & operator=(EC_SpatialProxy& proxy);
	void Register();
	void UnRegister();
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;
	std::shared_ptr<Spatial> spatial_component;
	glm::vec3 position,
		velocity,
		ang_vel,
		orientation;
	glm::mat4 transform;
	std::shared_ptr<GameEntity> e;

};

class EC_SpatialSystem :
	public EC_System
{
public:
	EC_SpatialSystem();
	virtual ~EC_SpatialSystem();

	// Inherited via EC_System
	virtual void init(ECXMessenger& queue, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game& game) override;
	virtual void addEntity(std::shared_ptr<GameEntity>& e) override;
	virtual void removeEntity(unsigned int entityID) override;
	virtual void clearEntities() override;
private:
	std::vector<EC_SpatialProxy> m_Entities;
};

