#pragma once
#include "EC_LuaScript.h"
class EC_OnKeyHeldHandler :
	public EC_Lua_Script
{
public:
	EC_OnKeyHeldHandler(const std::string& filename);
	virtual ~EC_OnKeyHeldHandler();

	// Inherited via EC_Lua_Script
	virtual void handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy & proxy) override;
};

