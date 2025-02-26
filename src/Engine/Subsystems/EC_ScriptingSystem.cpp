#include "EC_ScriptingSystem.h"
#include "Components/Spatial.h"
#include "Components/EC_ScriptComponent.h"
#include <exception>
#include <iostream>
#include "Messaging/ECXMessenger.h"

EC_ScriptingSystem::EC_ScriptingSystem()
{
}

EC_ScriptingSystem::~EC_ScriptingSystem()
{
}

void EC_ScriptingSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	std::vector<ECXEventType> types{	ECXEventType::EntityCreate,
										ECXEventType::EntityDestroy,
										ECXEventType::key_down,
										ECXEventType::key_up,
										ECXEventType::key_held,
										ECXEventType::mouse_down,
										ECXEventType::mouse_up,
										ECXEventType::mouse_move,
										ECXEventType::system_update
	};
	messenger.Subscribe(*this, types);
	m_game = &game;
}


void EC_ScriptingSystem::update(const float & deltaTimeS, EC_Game & game)
{

}

void EC_ScriptingSystem::addEntity(std::shared_ptr<GameEntity>& e)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	if (e->hasComponent<Spatial>() && e->hasComponent<EC_ScriptComponent>())
	{
		m_entities.emplace_back(e);
	}
}

void EC_ScriptingSystem::removeEntity(unsigned int entityID)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	for (size_t i = 0; i < m_entities.size(); i++)
	{
		if (m_entities[i].m_entity->getUID() == entityID)
		{
			m_entities[i].UnRegister();
			m_entities[i] = std::move(m_entities.back());
			m_entities.resize(m_entities.size() - 1);
			break;
		}
	}
}

void EC_ScriptingSystem::clearEntities()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_entities.clear();
}

void EC_ScriptingSystem::receive(ECXEvent& Event)
{
	for (auto iter = m_entities.begin(); iter != m_entities.end(); iter++)
	{
		(*iter).handleEvent(Event, *m_game);
	}
}
