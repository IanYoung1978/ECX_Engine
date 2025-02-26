#include "EC_TransformSystem.h"
#include <glm\gtc\matrix_transform.hpp>
#include <mutex>
#include "Components/ProxyHelper.h"

EC_TransformSystem::EC_TransformSystem()
{
}


EC_TransformSystem::~EC_TransformSystem()
{
}

void EC_TransformSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	//not used
}

void EC_TransformSystem::update(const float & deltaTimeS, EC_Game & game)
{
	{
		std::scoped_lock<std::mutex> lock(m_lock);
		if (!m_to_add.empty())
		{
			for (auto e : m_to_add)
			{
				m_entities.emplace_back(e);
			}
			m_to_add.clear();
		}
		if (!m_to_remove.empty())
		{
			for (size_t i = 0; i < m_to_remove.size(); i++)
			{
				for (size_t j = 0; j < m_entities.size(); j++)
				{
					if (m_entities[j].entity->getUID() == m_to_remove[i])
					{
						m_entities[j].UnRegister();
						m_entities[j] = std::move(m_entities.back());
						m_entities.resize(m_entities.size() - 1);
						break;
					}
				}
			}
			m_to_remove.clear();
		}
	}
	for (size_t i = 0; i < m_entities.size(); i++)
	{
		glm::mat4 trans(1.0f);
		trans = glm::translate(trans, m_entities[i].position);
		glm::mat4 rotation(1.0f);
		rotation = glm::rotate(rotation, m_entities[i].orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		rotation = glm::rotate(rotation, m_entities[i].orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		rotation = glm::rotate(rotation, m_entities[i].orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 scale(1.0f);
		scale = glm::scale(scale, m_entities[i].scale);
		glm::mat4 matrix = trans * rotation * scale;
		m_entities[i].transform->setTransform(matrix);
	}
}

void EC_TransformSystem::addEntity(std::shared_ptr<GameEntity>& e)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	if (e->hasComponent<Spatial>() && e->hasComponent<Transform>())
	{
		m_to_add.push_back(e);
	}
}

void EC_TransformSystem::removeEntity(unsigned int entityID)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_to_remove.push_back(entityID);
}

void EC_TransformSystem::clearEntities()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_entities.clear();
}

void EC_TransformSystem::Update()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	for (size_t i = 0; i < m_entities.size(); i++)
	{
		glm::mat4 trans(1.0f);
		trans = glm::translate(trans, m_entities[i].position);
		glm::mat4 rotation(1.0f);
		rotation = glm::rotate(rotation, m_entities[i].orientation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		rotation = glm::rotate(rotation, m_entities[i].orientation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		rotation = glm::rotate(rotation, m_entities[i].orientation.z, glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 scale(1.0f);
		scale = glm::scale(scale, m_entities[i].scale);
		glm::mat4 matrix = trans * rotation * scale;
		m_entities[i].transform->setTransform(matrix);
	}
}

void EC_TransformProxy::notify()
{
	position = spatial->getPosition();
	orientation = spatial->getOrientation();
	scale = transform->getScale();
}

bool EC_TransformProxy::operator==(EC_Observer & other)
{
	auto proxy = dynamic_cast<EC_ProxyObserver*>(&other);
	if (!proxy)
		return false;
	if (id == proxy->id)
		return true;
	return false;
}

EC_TransformProxy::EC_TransformProxy() :
	entity(nullptr),
	spatial(nullptr),
	transform(nullptr)
{
}

EC_TransformProxy::EC_TransformProxy(std::shared_ptr<GameEntity>& e) :
	entity(nullptr),
	spatial(nullptr),
	transform(nullptr)
{
	entity = e;
	id = ProxyHelper::getUID();
	spatial = entity->getComponent<Spatial>();
	transform = entity->getComponent<Transform>();
	position = spatial->getPosition();
	orientation = spatial->getOrientation();
	scale = transform->getScale();
	Register();
}

EC_TransformProxy::EC_TransformProxy(EC_TransformProxy && proxy) noexcept :
	entity(nullptr),
	spatial(nullptr),
	transform(nullptr)
{
	entity = proxy.entity;
	id = proxy.entity->getUID();
	spatial = proxy.entity->getComponent<Spatial>();
	transform = proxy.entity->getComponent<Transform>();
	position = proxy.spatial->getPosition();
	orientation = proxy.spatial->getOrientation();
	scale = proxy.transform->getScale();
	proxy.UnRegister();
	Register();
}

EC_TransformProxy::EC_TransformProxy(EC_TransformProxy & proxy)
{
	entity = proxy.entity;
	id = proxy.entity->getUID();
	spatial = proxy.entity->getComponent<Spatial>();
	transform = proxy.entity->getComponent<Transform>();
	position = proxy.spatial->getPosition();
	orientation = proxy.spatial->getOrientation();
	scale = proxy.transform->getScale();
	proxy.UnRegister();
	Register();
}

EC_TransformProxy & EC_TransformProxy::operator=(EC_TransformProxy & proxy)
{
	entity = proxy.entity;
	id = proxy.entity->getUID();
	spatial = proxy.entity->getComponent<Spatial>();
	transform = proxy.entity->getComponent<Transform>();
	position = proxy.spatial->getPosition();
	orientation = proxy.spatial->getOrientation();
	scale = proxy.transform->getScale();
	proxy.UnRegister();
	Register();
	return *this;
}

void EC_TransformProxy::Register()
{
	spatial->Register(this);
	transform->Register(this);
}

void EC_TransformProxy::UnRegister()
{
	spatial->UnRegister(this);
	transform->UnRegister(this);
}
