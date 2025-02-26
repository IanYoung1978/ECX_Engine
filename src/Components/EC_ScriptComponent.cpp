#include "EC_ScriptComponent.h"
#include "Messaging/KeyEvent.h"
#include "Logging/ECX_Logging.h"

EC_ScriptComponent::EC_ScriptComponent()
{
}

bool EC_ScriptComponent::setBehaviour(EC_BehaviourType type, std::shared_ptr<EC_Lua_Script> script)
{
	m_behaviours.emplace(type, script);
	if (m_behaviours[type]->init())
		return true;
	return false;
}

std::shared_ptr<EC_Lua_Script> EC_ScriptComponent::getBehaviour(EC_BehaviourType type)
{
	auto s = m_behaviours.find(type);
	if (s != m_behaviours.end())
		return (s->second);
	return nullptr;
}

void EC_ScriptComponent::handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy)
{
	switch (e.type)
	{
	case ECXEventType::key_down:
		m_behaviours[EC_BehaviourType::key_down]->handleEvent(e, game, proxy);
		break;
	case ECXEventType::key_up:
		m_behaviours[EC_BehaviourType::key_up]->handleEvent(e, game, proxy);
		break;
	case ECXEventType::key_held:
		m_behaviours[EC_BehaviourType::key_held]->handleEvent(e, game, proxy);
		break;
	case ECXEventType::mouse_move:
		m_behaviours[EC_BehaviourType::mouse_move]->handleEvent(e, game, proxy);
		break;
	default:
		LOGGING::ECX_Logger::GetInstance()->LogMessage("UNSUPPORTED EVENT: ", LOGGING::LogLevel::TRIVIAL);
		break;
	}
}

bool EC_ScriptComponent::hasBehaviour(EC_BehaviourType type)
{
	auto s = m_behaviours.find(type);
	if (s != m_behaviours.end())
		return true;
	return false;
}


EC_ScriptComponent::~EC_ScriptComponent()
{
}

ComponentType EC_ScriptComponent::getComponentType()
{
	return ComponentType::Script;
}
