#pragma once
#include "EC_LuaScript.h"
class EC_OnKeyUpHandler :
	public EC_Lua_Script
{
public:
	EC_OnKeyUpHandler(const std::string& filename);
	virtual ~EC_OnKeyUpHandler();

	// Inherited via EC_Lua_Script
	virtual void handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy) override;
};

