#include "EC_OnMouseMoveHandler.h"
#include "Messaging/MouseEvent.h"
#include <iostream>
#include "Logging/ECX_Logging.h"

EC_OnMouseMoveHandler::EC_OnMouseMoveHandler(const std::string& filename) :EC_Lua_Script(filename)
{
}


EC_OnMouseMoveHandler::~EC_OnMouseMoveHandler()
{
}

void EC_OnMouseMoveHandler::handleEvent(ECXEvent& e, EC_Game& game, EC_LuaScriptProxy& proxy)
{
	MouseEvent m = std::any_cast<MouseEvent>(e.args[0]);
	GameProxy g(game);
	luabridge::LuaRef script = luabridge::getGlobal(m_state, "onMouseMove");
	try
	{
		auto retval = script(m, proxy, g);
		if (!retval)
		{
			LOGGING::ECX_Logger::GetInstance()->LogMessage("Error in onMouseMove script: " + retval.errorMessage(), LOGGING::LogLevel::CRITICAL);
		}
	}
	catch (luabridge::LuaException const& e)
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Script exception: " + std::string(e.what()), LOGGING::LogLevel::SEVERE);
	}
}
