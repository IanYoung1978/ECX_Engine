#include "EC_Lua_Entity_Proxy.h"
#include "Spatial.h"
#include "EC_ScriptComponent.h"
#include <lua.hpp>
#include <luabridge3/LuaBridge/LuaBridge.h>

#include "Entity/GameEntity.h"
#include "Messaging/KeyEvent.h"
#include "Components/ProxyHelper.h"

EC_Lua_Entity_Proxy::EC_Lua_Entity_Proxy(std::shared_ptr<GameEntity> e)
{
	id = ProxyHelper::getUID();
	m_scripts = e->getComponent<EC_ScriptComponent>();
	m_spatial = e->getComponent<Spatial>();
	m_entity = e;
	m_scripts->Register(this);
	m_script_proxy = EC_LuaScriptProxy(e);
}

EC_Lua_Entity_Proxy::~EC_Lua_Entity_Proxy()
{

}

EC_Lua_Entity_Proxy::EC_Lua_Entity_Proxy(EC_Lua_Entity_Proxy && proxy) noexcept
{
	id = proxy.id;
	m_scripts = proxy.m_scripts;
	m_spatial = proxy.m_spatial;
	m_entity = proxy.m_entity;
	m_script_proxy = proxy.m_script_proxy;
	proxy.UnRegister();
	m_scripts->Register(this);
}

EC_Lua_Entity_Proxy & EC_Lua_Entity_Proxy::operator=(EC_Lua_Entity_Proxy & proxy)
{
	id = proxy.id;
	m_scripts = proxy.m_scripts;
	m_spatial = proxy.m_spatial;
	m_entity = proxy.m_entity;
	proxy.UnRegister();
	m_scripts->Register(this);
	return *this;
}

void EC_Lua_Entity_Proxy::Register()
{
	m_scripts->Register(this);
}

void EC_Lua_Entity_Proxy::UnRegister()
{
	m_scripts->UnRegister(this);
}

void EC_Lua_Entity_Proxy::handleEvent(ECXEvent& e, EC_Game & game)
{
	m_script_proxy.handleEvent(e, game);
}

void EC_Lua_Entity_Proxy::notify()
{
	m_scripts = m_entity->getComponent<EC_ScriptComponent>();
	m_spatial = m_entity->getComponent<Spatial>();
}

bool EC_Lua_Entity_Proxy::operator==(EC_Observer & other)
{
	return false;
}
