#include "EntityManager.h"
#include "Logging/ECX_Logging.h"

EntityManager::EntityManager()
{
}

EntityManager::~EntityManager()
{
}


EntityManager& EntityManager::getInstance()
{
	if (m_instance == nullptr)
	{
		m_instance = new EntityManager();
	}
	return *m_instance;
}

void EntityManager::addEntity(std::shared_ptr<GameEntity> entity)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_entities.push_back(entity);
}

void EntityManager::removeEntity(std::shared_ptr<GameEntity> entity)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	auto it = std::find(m_entities.begin(), m_entities.end(), entity);
	if (it != m_entities.end())
	{
		m_entities.erase(it);
	}
	else
	{
		// Entity not found	
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Entity not found", LOGGING::LogLevel::SEVERE);
	}
}

void EntityManager::removeEntity(std::string name)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	auto it = std::find_if(m_entities.begin(), m_entities.end(), [name](std::shared_ptr<GameEntity> entity) { return entity->getName() == name; });
	if (it != m_entities.end())
	{
		m_entities.erase(it);
	}
	else
	{
		// Entity not found	
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Entity not found", LOGGING::LogLevel::SEVERE);
	}
}

std::shared_ptr<GameEntity> EntityManager::getEntity(std::string name)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	auto it = std::find_if(m_entities.begin(), m_entities.end(), [name](std::shared_ptr<GameEntity> entity) { return entity->getName() == name; });
	if (it != m_entities.end())
	{
		return *it;
	}

	// Entity not found	
	LOGGING::ECX_Logger::GetInstance()->LogMessage("Entity not found", LOGGING::LogLevel::SEVERE);

	return nullptr;
}

std::vector<std::shared_ptr<GameEntity>>& EntityManager::getEntities()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	return m_entities;
}

std::vector<std::shared_ptr<GameEntity>> EntityManager::getEntitiesWithComponent(std::type_index type)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	std::vector<std::shared_ptr<GameEntity>> entities;
	for (auto entity : m_entities)
	{
		if (entity->hasComponent(type))
		{
			entities.push_back(entity);
		}
	}
	return entities;
}

std::vector<std::shared_ptr<GameEntity>> EntityManager::getEntitiesWithComponents(std::vector<std::type_index> types)
{
	// TODO: Optimize this function
	// Current implementation checks each entity for all component types
	// which can be inefficient for large numbers of entities and component types
	// Consider using a more efficient data structure or indexing method
	// to keep track of entities by their components
	// Bitset or component masks could be a solution
	std::scoped_lock<std::mutex> lock(m_lock);
	std::vector<std::shared_ptr<GameEntity>> entities;
	for (auto entity : m_entities)
	{
		bool hasAllComponents = true;
		for (auto type : types)
		{
			if (!entity->hasComponent(type))
			{
				hasAllComponents = false;
				break;
			}
		}
		if (hasAllComponents)
		{
			entities.push_back(entity);
		}
	}
	return entities;
}

void EntityManager::clearEntities()
{
	m_entities.clear();
}

EntityManager* EntityManager::m_instance = nullptr;