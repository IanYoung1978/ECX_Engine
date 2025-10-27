#pragma once
#include "EC_System.h"
#include "Entity/GameEntity.h"
#include <memory>
#include <lua.hpp>
#include <LuaBridge3/LuaBridge/LuaBridge.h>
#include "Components/EC_Lua_Entity_Proxy.h"
#include <deque>
#include "Messaging/IEventListener.h"


class EC_ScriptingSystem :
	public EC_System, public IEventListener
{
public:
	EC_ScriptingSystem();
	virtual ~EC_ScriptingSystem();

	// Inherited via EC_System
	virtual void init(ECXMessenger& messenger, EC_Game& game) override;
	virtual void update(const float & deltaTimeS, EC_Game & game) override;
	// Inherited via IEventListener
	void receive(ECXEvent& Event) override;
private:
	std::deque<std::shared_ptr<EC_Event>> m_Events;
	EC_Game* m_game;

};

