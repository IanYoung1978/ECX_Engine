#pragma once
#include "EC_LuaScript.h"
class EC_OnCreateHandler :
	public EC_Lua_Script
{
public:
	EC_OnCreateHandler(const std::string& filename);
	virtual ~EC_OnCreateHandler();

	// Inherited via EC_Lua_Script
	virtual void handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy) override;
};

