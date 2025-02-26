#include "GameEntity.h"



GameEntity::GameEntity()
{
	m_UID = 0;
	m_Active = false;
}

void GameEntity::addComponent(std::type_index type, std::shared_ptr<IComponent> component)
{
	m_Components[type] = component;
}

void GameEntity::setName(std::string name)
{
	m_Name = name;
}

std::string GameEntity::getName()
{
	return m_Name;
}

unsigned int GameEntity::getUID()
{
	return m_UID;
}

void GameEntity::setUID(unsigned int uid)
{
	m_UID = uid;
}

GameEntity::~GameEntity()
{
	for (auto c : m_Components)
	{
		c.second.reset();
	}
}
