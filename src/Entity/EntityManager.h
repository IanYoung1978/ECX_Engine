#pragma once

#include "GameEntity.h"
#include <vector>
#include <memory>
#include <mutex>


class EntityManager
{
public:
	EntityManager(const EntityManager&) = delete;
	EntityManager& operator=(const EntityManager&) = delete;
	static EntityManager& getInstance();
	void addEntity(std::shared_ptr<GameEntity> entity);
	void removeEntity(std::shared_ptr<GameEntity> entity);
	void removeEntity(std::string name);
	void clearEntities();
	std::shared_ptr<GameEntity> getEntity(std::string name);
	std::vector<std::shared_ptr<GameEntity>>& getEntities();
	std::vector<std::shared_ptr<GameEntity>> getEntitiesWithComponent(std::type_index type);
	std::vector<std::shared_ptr<GameEntity>> getEntitiesWithComponents(std::vector<std::type_index> types);
	virtual ~EntityManager();
private:
	EntityManager();
	std::vector<std::shared_ptr<GameEntity>> m_entities;
	std::mutex m_lock;
	static EntityManager* m_instance;
};