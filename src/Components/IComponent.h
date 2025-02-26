#pragma once

#include "ComponentType.h"
#include "Common/EC_Subject.h"
class GameEntity;
class IComponent:
	public EC_Subject
{
public:
	IComponent();
	void setOwner(std::shared_ptr<GameEntity>& entity);
	std::shared_ptr<GameEntity>& getOwner();
	virtual ComponentType getComponentType() = 0;
	~IComponent();
protected:

	std::shared_ptr<GameEntity> m_Owner;
};

