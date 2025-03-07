#include "EC_OnCreateHandler.h"
#include "Logging/ECX_Logging.h"


EC_OnCreateHandler::EC_OnCreateHandler(const std::string& filename) :EC_Lua_Script(filename)
{
}


EC_OnCreateHandler::~EC_OnCreateHandler()
{
}

void EC_OnCreateHandler::handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy)
{
	if (e.type == ECXEventType::EntityCreate) 
	{
		auto script = luabridge::getGlobal(m_state, "onCreate");
		luabridge::LuaResult retval = script(e, proxy, GameProxy(game));
		if (!retval)
		{
			LOGGING::ECX_Logger::GetInstance()->LogMessage("Error in OnCreate script: " + retval.errorMessage(), LOGGING::LogLevel::CRITICAL);
		}
	}
}
