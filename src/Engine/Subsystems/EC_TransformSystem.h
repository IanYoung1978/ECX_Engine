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

class EC_TransformProxy:
	public EC_ProxyObserver
{
public:
	EC_TransformProxy();
	EC_TransformProxy(std::shared_ptr<GameEntity>& e);
	EC_TransformProxy(EC_TransformProxy&& proxy) noexcept;
	EC_TransformProxy(EC_TransformProxy& proxy);
	EC_TransformProxy& operator=(EC_TransformProxy& proxy);
	void Register();
	void UnRegister();
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 orientation;
	std::shared_ptr<Spatial> spatial;
	std::shared_ptr<Transform> transform;
	std::shared_ptr<GameEntity> entity;
};
class EC_TransformSystem :
	public EC_System
{
public:
	EC_TransformSystem();
	virtual ~EC_TransformSystem();

	// Inherited via EC_System
	virtual void init(ECXMessenger& messenger, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game & game) override;
	virtual void addEntity(std::shared_ptr<GameEntity>& e) override;
	virtual void removeEntity(unsigned int entityID) override;
	virtual void clearEntities() override;
private:
	void Update();
	std::vector<EC_TransformProxy> m_entities;


};

