#include "IComponent.h"



IComponent::IComponent()
{
}

void IComponent::setOwner(std::shared_ptr<GameEntity>& entity)
{
	m_Owner = entity;
}

std::shared_ptr<GameEntity>& IComponent::getOwner()
{
	return m_Owner;
}


IComponent::~IComponent()
{
}
