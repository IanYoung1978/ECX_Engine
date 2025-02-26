#pragma once
#include <string>
#include <memory>
#include <lua.hpp>
#include <luabridge3/LuaBridge/LuaBridge.h>

#include "Game.h"
#include "EC_Lua_Entity_Proxy.h"
#include "Messaging/ECXEvent.h"

class KeyEvent;
class EC_Game;
class EC_Lua_Entity_Proxy;

class GameProxy
{
public:
	GameProxy(EC_Game& game)
	{
		m_game = &game;
	}

	EC_LuaScriptProxy getEntity(const std::string& entityName)
	{
		auto e = m_game->getEntityByName(entityName);
		if (e != nullptr)
		{
			return EC_LuaScriptProxy(e);
		}
		return nullptr;
	}
	void shutDown() 
	{
		m_game->shutDown();
	}
	void changeMode(const std::string& mode_name) 
	{
		if (mode_name == "Intro")
		{
			m_game->changeMode(Game_Mode::Intro);
		}
		else if (mode_name == "Menu")
		{
			m_game->changeMode(Game_Mode::Menu);
		}
		else if (mode_name == "Game")
		{
			m_game->changeMode(Game_Mode::Game);
		}
		else if (mode_name == "Credits")
		{
			m_game->changeMode(Game_Mode::Credits);
		}
	}
	unsigned int getKeyState(const std::string& keyname)
	{
		SDL_Scancode key = SDL_GetScancodeFromName(keyname.c_str());
		KeyState state = m_game->getKeyState(key);
		return (unsigned int)state;
	}
private:
	EC_Game * m_game;
};

class EC_Lua_Script
{
public:
	EC_Lua_Script();
	virtual ~EC_Lua_Script();
	bool init();
	virtual void handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy) = 0;
protected:
	EC_Lua_Script(const std::string& filename);
	lua_State * m_state;
	std::string m_scriptFile;
};