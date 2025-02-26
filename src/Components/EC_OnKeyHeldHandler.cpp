#include "EC_OnKeyHeldHandler.h"
#include "Messaging/KeyEvent.h"
#include <iostream>
#include "Logging/ECX_Logging.h"

EC_OnKeyHeldHandler::EC_OnKeyHeldHandler(const std::string& filename) :EC_Lua_Script(filename)
{
}

EC_OnKeyHeldHandler::~EC_OnKeyHeldHandler()
{
}

void EC_OnKeyHeldHandler::handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy & proxy)
{
	KeyEvent key = std::any_cast<KeyEvent>(e.args[0]);
	GameProxy g(game);
	try
	{
		luabridge::LuaRef script = luabridge::getGlobal(m_state, "onKeyHeld");
		auto retval = script(key, proxy, g);
	}
	catch (std::exception const& e)
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Script exception: " + std::string(e.what()), LOGGING::LogLevel::SEVERE);
	}
}
