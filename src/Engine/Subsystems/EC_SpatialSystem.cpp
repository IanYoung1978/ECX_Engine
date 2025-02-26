#include "EC_SpatialSystem.h"
#include "Components/Spatial.h"
#include "Components/Transform.h"
#include <glm\gtc\matrix_transform.hpp>
#include "Common/EC_Subject.h"
#include <glm\gtc\quaternion.hpp>
#include <iostream>
#include "Components/ProxyHelper.h"

EC_SpatialSystem::EC_SpatialSystem()
{
}


EC_SpatialSystem::~EC_SpatialSystem()
{
	m_Entities.clear();
}

void EC_SpatialSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	// not used
}

void EC_SpatialSystem::update(const float & deltaTimeS, EC_Game& game)
{
	{
		std::scoped_lock<std::mutex> lock(m_lock);
		if (!m_to_add.empty())
		{
			for (auto e : m_to_add)
			{
				m_Entities.emplace_back(e);
			}
			m_to_add.clear();
		}
		if (!m_to_remove.empty())
		{
			for (size_t i = 0; i < m_to_remove.size(); i++)
			{
				for (size_t j = 0; j < m_Entities.size(); j++)
				{
					if (m_Entities[j].e->getUID() == m_to_remove[i])
					{
						m_Entities[j].UnRegister();
						m_Entities[j] = std::move(m_Entities.back());
						m_Entities.resize(m_Entities.size() - 1);
						break;
					}
				}
			}
			m_to_remove.clear();
		}
	}
	for (size_t i = 0; i < m_Entities.size(); i++)
	{
		//compute changes
		m_Entities[i].position += m_Entities[i].velocity*deltaTimeS;
		m_Entities[i].orientation += m_Entities[i].ang_vel*deltaTimeS;

		glm::vec3 Direction;
		Direction.x = cos(m_Entities[i].orientation.x) * sin(m_Entities[i].orientation.y);
		Direction.y = sin(m_Entities[i].orientation.x);
		Direction.z = cos(m_Entities[i].orientation.x) * cos(m_Entities[i].orientation.y);
		Direction = glm::normalize(Direction);
		glm::vec3 Right = glm::normalize(glm::cross(Direction, glm::vec3(0.0f,1.0f,0.0f)));
		glm::vec3 Up = glm::normalize(glm::cross(Right, Direction));
		//update components
		m_Entities[i].spatial_component->setOrientation(m_Entities[i].orientation);
		m_Entities[i].spatial_component->setPosition(m_Entities[i].position);
		m_Entities[i].spatial_component->setDirections(Direction, Up, Right);

	}

}

void EC_SpatialSystem::addEntity(std::shared_ptr<GameEntity>& e)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	
	if (e->hasComponent<Spatial>())
	{
		m_to_add.push_back(e);
	}
}

void EC_SpatialSystem::removeEntity(unsigned int entityID)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_to_remove.push_back(entityID);
}

void EC_SpatialSystem::clearEntities()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_Entities.clear();
}

EC_SpatialProxy::EC_SpatialProxy(std::shared_ptr<GameEntity> entity)
{
	id = ProxyHelper::getUID();
	e = entity;
	spatial_component = entity->getComponent<Spatial>();
	position = spatial_component->getPosition();
	velocity = spatial_component->getVelocity();
	ang_vel = spatial_component->getAngVelocity();
	orientation = spatial_component->getOrientation();
	spatial_component->Register(this);
}

EC_SpatialProxy::~EC_SpatialProxy()
{
}

EC_SpatialProxy::EC_SpatialProxy(EC_SpatialProxy && proxy) noexcept
{
	id = proxy.id;
	spatial_component = proxy.spatial_component;
	position = spatial_component->getPosition();
	velocity = spatial_component->getVelocity();
	ang_vel = spatial_component->getAngVelocity();
	orientation = spatial_component->getOrientation();
	proxy.UnRegister();
	spatial_component->Register(this);
}

EC_SpatialProxy & EC_SpatialProxy::operator=(EC_SpatialProxy & proxy)
{
	id = proxy.id;
	spatial_component = proxy.spatial_component;
	position = spatial_component->getPosition();
	velocity = spatial_component->getVelocity();
	ang_vel = spatial_component->getAngVelocity();
	orientation = spatial_component->getOrientation();
	proxy.UnRegister();
	Register();
	return *this;
}

void EC_SpatialProxy::Register()
{
	spatial_component->Register(this);
}

void EC_SpatialProxy::UnRegister()
{
	spatial_component->UnRegister(this);
}

void EC_SpatialProxy::notify()
{
	position = spatial_component->getPosition();
	velocity = spatial_component->getVelocity();
	ang_vel = spatial_component->getAngVelocity();
	orientation = spatial_component->getOrientation();
}

bool EC_SpatialProxy::operator==(EC_Observer & other)
{
	auto proxy = dynamic_cast<EC_ProxyObserver*>(&other);
	if (!proxy)
		return false;
	if (id == proxy->id)
		return true;
	return false;
}
