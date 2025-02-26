#include "EC_OnKeyUpHandler.h"
#include "Messaging/KeyEvent.h"
#include <iostream>
#include <exception>
#include "Logging/ECX_Logging.h"

EC_OnKeyUpHandler::EC_OnKeyUpHandler(const std::string& filename) :EC_Lua_Script(filename)
{
}


EC_OnKeyUpHandler::~EC_OnKeyUpHandler()
{
}

void EC_OnKeyUpHandler::handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy)
{
	KeyEvent key = std::any_cast<KeyEvent>(e.args[0]);

	GameProxy g(game);
	try
	{
		luabridge::LuaRef script = luabridge::getGlobal(m_state, "onKeyUp");
		auto retval = script(key, proxy, g);
	}
	catch (std::exception const& e)
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Script exception: " + std::string(e.what()), LOGGING::LogLevel::SEVERE);
	}
}
