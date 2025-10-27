#include "EC_ScriptingSystem.h"
#include "Components/Spatial.h"
#include "Components/EC_ScriptComponent.h"
#include <exception>
#include <iostream>
#include "Messaging/ECXMessenger.h"
#include "Entity/EntityManager.h"

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

void EC_ScriptingSystem::receive(ECXEvent& Event)
{
	auto entities = EntityManager::getInstance().getEntitiesWithComponent(std::type_index(typeid(EC_ScriptComponent)));
	for (auto e : entities)
	{
		EC_Lua_Entity_Proxy proxy(e);
		proxy.handleEvent(Event, *m_game);
	}
}
