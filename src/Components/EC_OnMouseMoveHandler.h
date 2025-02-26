#pragma once
#include "EC_LuaScript.h"

class EC_OnMouseMoveHandler :
	public EC_Lua_Script
{
public:
	EC_OnMouseMoveHandler(const std::string& filename);
	virtual ~EC_OnMouseMoveHandler();

	// Inherited via EC_Lua_Script
	virtual void handleEvent(ECXEvent& e, EC_Game& game, EC_LuaScriptProxy& proxy) override;
};

