#pragma once
#include "Common/EC_ProxyObserver.h"
#include "EC_LuaScriptProxy.h"
#include <glm\glm.hpp>
#include <memory>
#include "Game.h"
#include "Messaging/ECXEvent.h"

class GameEntity;
class Spatial;
class EC_ScriptComponent;


// a proxy for game entities
class EC_Lua_Entity_Proxy
	:public EC_ProxyObserver
{
public:

	EC_Lua_Entity_Proxy() { ; }
	EC_Lua_Entity_Proxy(std::shared_ptr<GameEntity> e);
	~EC_Lua_Entity_Proxy();
	EC_Lua_Entity_Proxy(EC_Lua_Entity_Proxy&& proxy) noexcept;
	EC_Lua_Entity_Proxy & operator=(EC_Lua_Entity_Proxy& proxy);
	void Register();
	void UnRegister();

	void handleEvent(ECXEvent& e, EC_Game & game);
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;
	std::shared_ptr<EC_ScriptComponent> m_scripts;
	std::shared_ptr<Spatial> m_spatial;
	std::shared_ptr<GameEntity> m_entity;
	EC_LuaScriptProxy m_script_proxy;
};
