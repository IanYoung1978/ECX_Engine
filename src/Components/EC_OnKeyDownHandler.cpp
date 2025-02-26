#include "EC_OnKeyDownHandler.h"
#include "Messaging/KeyEvent.h"
#include <iostream>
#include "Logging/ECX_Logging.h"

EC_OnKeyDownHandler::EC_OnKeyDownHandler(const std::string& filename):EC_Lua_Script(filename)
{
}


EC_OnKeyDownHandler::~EC_OnKeyDownHandler()
{
}

void EC_OnKeyDownHandler::handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy)
{
	KeyEvent key = std::any_cast<KeyEvent>(e.args[0]);

	GameProxy g(game);
	try
	{
		luabridge::LuaRef script = luabridge::getGlobal(m_state, "onKeyDown");
		auto retval = script(key, proxy, g);
	}
	catch (std::exception const& e)
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Script exception: " + std::string(e.what()), LOGGING::LogLevel::SEVERE);
	}
}
