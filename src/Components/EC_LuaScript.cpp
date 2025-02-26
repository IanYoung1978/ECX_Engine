#include "EC_LuaScript.h"
#include "EC_LuaScriptProxy.h"
#include <iostream>
#include "Messaging/KeyEvent.h"
#include "Messaging/MouseEvent.h"
#include <lua.hpp>
#include "Logging/ECX_Logging.h"

EC_Lua_Script::EC_Lua_Script(const std::string& filename)
{
	m_scriptFile = filename;
	m_state = nullptr;
}

EC_Lua_Script::EC_Lua_Script()
{
	m_state = nullptr;
}

EC_Lua_Script::~EC_Lua_Script()
{
	if (m_state)
		lua_close(m_state);
}
bool EC_Lua_Script::init()
{
	if (m_scriptFile == "")
		return false;
	m_state = luaL_newstate();

	// Make standard libraries available in the Lua object
	luaL_openlibs(m_state);
	int ret = luaL_dofile(m_state, m_scriptFile.c_str());
	luabridge::getGlobalNamespace(m_state)
		.beginClass<EC_LuaScriptProxy>("entity")
			.addProperty("position", &EC_LuaScriptProxy::getPosition, &EC_LuaScriptProxy::setPosition)
			.addProperty("velocity", &EC_LuaScriptProxy::getVelocity, &EC_LuaScriptProxy::setVelocity)
			.addProperty("orientation", &EC_LuaScriptProxy::getOrientation, &EC_LuaScriptProxy::setOrientation)
			.addProperty("rotation", &EC_LuaScriptProxy::getRotation, &EC_LuaScriptProxy::setRotation)
			.addFunction("forward", &EC_LuaScriptProxy::getForward)
			.addFunction("up", &EC_LuaScriptProxy::getUp)
			.addFunction("right", &EC_LuaScriptProxy::getRight)
			.addFunction("moveForward", &EC_LuaScriptProxy::moveForward)
			.addFunction("moveBack", &EC_LuaScriptProxy::moveBack)
			.addFunction("moveLeft", &EC_LuaScriptProxy::moveLeft)
			.addFunction("moveRight", &EC_LuaScriptProxy::moveRight)
			.addFunction("moveUp", &EC_LuaScriptProxy::moveUp)
			.addFunction("moveDown", &EC_LuaScriptProxy::moveDown)
			.addFunction("rotateAroundAxis", &EC_LuaScriptProxy::rotateAroundAxis)
		.endClass()
		.beginClass<Vec3>("vec3")
			.addConstructor<void(*)(const float&, const float&, const float&)>()
			.addProperty("x", &Vec3::x, true)
			.addProperty("y", &Vec3::y, true)
			.addProperty("z", &Vec3::z, true)
			.addStaticFunction("length", &Vec3::length)
			.addStaticFunction("normalize", &Vec3::normalize)
			.addFunction("__add", &Vec3::operator+=)
			.addFunction("__sub", &Vec3::operator-=)
			.addFunction("__mul", &Vec3::operator*=)
			.addFunction("__div", &Vec3::operator/=)
		.endClass()
		.beginClass<GameProxy>("game")
			.addFunction("getEntity",&GameProxy::getEntity)
			.addFunction("getKeyState", &GameProxy::getKeyState)
			.addFunction("shutdown", &GameProxy::shutDown)
		.endClass()
		.beginClass<KeyEvent>("key_event")
			.addFunction("is_pressed", &KeyEvent::isPressed)
			.addFunction("is_held", &KeyEvent::isHeld)
			.addFunction("is_released", &KeyEvent::isReleased)
			.addFunction("key",&KeyEvent::getKeyString)
		.endClass()
		.beginClass<MouseEvent>("mouse_event")
			.addFunction("x", &MouseEvent::getXMotion)
			.addFunction("y", &MouseEvent::getYMotion)
		.endClass();

	if (ret) {
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Error: script not loaded: " + m_scriptFile, LOGGING::LogLevel::SEVERE);
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Error occurs when calling luaL_loadfile" + m_scriptFile, LOGGING::LogLevel::SEVERE);
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Error: " + std::string(lua_tostring(m_state, -1)), LOGGING::LogLevel::SEVERE);
	
		m_state = 0;
		return false;
	}

	LOGGING::ECX_Logger::GetInstance()->LogMessage("Script: " + m_scriptFile + " loaded.", LOGGING::LogLevel::INFORMATION);
	return true;
}
