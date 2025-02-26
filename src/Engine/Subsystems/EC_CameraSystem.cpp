#include "EC_CameraSystem.h"
#include "Components/EC_CameraComponent.h"
#include "Components/Spatial.h"
#include <glm\gtc\matrix_transform.hpp>
#include "Entity/GameEntity.h"
#include "Components/ProxyHelper.h"

EC_CameraSystem::EC_CameraSystem()
{
}


EC_CameraSystem::~EC_CameraSystem()
{
}

void EC_CameraSystem::init(ECXMessenger& messenger, EC_Game& game)
{
}

void EC_CameraSystem::update(const float & deltaTimeS, EC_Game & game)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	for (size_t i = 0; i < m_entities.size(); i++)
	{
		if (m_entities[i].isActive)
		{
			glm::vec3 pos = m_entities[i].position;
			glm::vec3 forward = m_entities[i].spatial->getForward();
			glm::vec3 up = m_entities[i].spatial->getUp();
			glm::mat4 view = glm::lookAt(pos, pos + forward, up);
			m_entities[i].cam->setViewMatrix(view);
		}
	}
}

void EC_CameraSystem::addEntity(std::shared_ptr<GameEntity>& e)
{

	if (e->hasComponent<Spatial>() && e->hasComponent<EC_CameraComponent>())
	{
		m_entities.emplace_back(e);
	}
}

void EC_CameraSystem::removeEntity(unsigned int entityID)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	for (size_t i = 0; i < m_entities.size(); i++)
	{
		if (m_entities[i].owner->getUID() == entityID)
		{
			m_entities[i].UnRegister();
			m_entities[i] = std::move(m_entities.back());
			m_entities.resize(m_entities.size() - 1);
			break;
		}
	}
}

void EC_CameraSystem::clearEntities()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_entities.clear();
}

EC_CameraSystemProxy::EC_CameraSystemProxy():
	draw_distance(0.0f),
	fov(0.0f),
	view_h(0),
	view_w(0),
	isActive(false)
{
	id = 0;
}

EC_CameraSystemProxy::EC_CameraSystemProxy(std::shared_ptr<GameEntity>& e) :
	draw_distance(0.0f),
	fov(0.0f),
	view_h(0),
	view_w(0),
	isActive(false)
{
	owner = e;
	spatial = e->getComponent<Spatial>();
	cam = e->getComponent<EC_CameraComponent>();
	id = ProxyHelper::getUID();
	position = spatial->getPosition();
	orientation = spatial->getOrientation();
	fov = cam->getFOVAngle();
	draw_distance = cam->getDrawDistance();
	cam->getViewPort(view_w, view_h);
	isActive = e->isActive();
	Register();
}

EC_CameraSystemProxy::~EC_CameraSystemProxy()
{
}

EC_CameraSystemProxy::EC_CameraSystemProxy(EC_CameraSystemProxy && proxy) noexcept :
	draw_distance(0.0f),
	fov(0.0f),
	view_h(0),
	view_w(0),
	isActive(false)
{
	owner = proxy.owner;
	spatial = proxy.spatial;
	cam = proxy.cam;
	id = proxy.id;
	position = spatial->getPosition();
	orientation = spatial->getOrientation();
	fov = cam->getFOVAngle();
	draw_distance = cam->getDrawDistance();
	isActive = proxy.isActive;
	proxy.UnRegister();
	Register();
}

EC_CameraSystemProxy::EC_CameraSystemProxy(EC_CameraSystemProxy & proxy) :
	draw_distance(0.0f),
	fov(0.0f),
	view_h(0),
	view_w(0),
	isActive(false)
{
	owner = proxy.owner;
	spatial = proxy.spatial;
	cam = proxy.cam;
	id = proxy.id;
	position = spatial->getPosition();
	orientation = spatial->getOrientation();
	fov = cam->getFOVAngle();
	draw_distance = cam->getDrawDistance();
	isActive = proxy.isActive;
	proxy.UnRegister();
	Register();
}

EC_CameraSystemProxy & EC_CameraSystemProxy::operator=(EC_CameraSystemProxy & proxy)
{
	owner = proxy.owner;
	spatial = proxy.spatial;
	cam = proxy.cam;
	id = proxy.id;
	position = spatial->getPosition();
	orientation = spatial->getOrientation();
	fov = cam->getFOVAngle();
	draw_distance = cam->getDrawDistance();
	proxy.UnRegister();
	Register();
	return *this;
}

void EC_CameraSystemProxy::Register()
{
	cam->Register(this);
}

void EC_CameraSystemProxy::UnRegister()
{
	cam->UnRegister(this);
}

void EC_CameraSystemProxy::notify()
{
	cam->getViewPort(view_w, view_h);
	fov = cam->getFOVAngle();
	draw_distance = cam->getDrawDistance();
	orientation = spatial->getOrientation(); //this needs to be a quaternion
	position = spatial->getPosition();
}

bool EC_CameraSystemProxy::operator==(EC_Observer & other)
{
	auto proxy = dynamic_cast<EC_CameraSystemProxy*>(&other);
	if (!proxy)
		return false;
	if (id == proxy->id)
		return true;
	return false;
}
