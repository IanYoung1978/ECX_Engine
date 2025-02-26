#pragma once
#include "EC_LuaScript.h"
class EC_OnKeyDownHandler :
	public EC_Lua_Script
{
public:
	EC_OnKeyDownHandler(const std::string& filename);
	virtual ~EC_OnKeyDownHandler();

	// Inherited via EC_Lua_Script
	virtual void handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy) override;
};

