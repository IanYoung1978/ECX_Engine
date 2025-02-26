#pragma once
#include "IComponent.h"
#include <vector>
#include <memory>
#include <string>
#include <map>
#include "EC_BehaviourType.h"
#include "EC_LuaScript.h"
#include "Messaging/ECXEvent.h"

class EC_LuaScriptProxy;
class EC_ScriptComponent :
	public IComponent
{
public:
	EC_ScriptComponent();
	bool setBehaviour(EC_BehaviourType type, std::shared_ptr<EC_Lua_Script> script);
	std::shared_ptr<EC_Lua_Script> getBehaviour(EC_BehaviourType type);
	void handleEvent(ECXEvent& e, EC_Game & game, EC_LuaScriptProxy& proxy);
	bool hasBehaviour(EC_BehaviourType type);
	virtual ~EC_ScriptComponent();

	// Inherited via IComponent
	virtual ComponentType getComponentType() override;
private:
	std::map<EC_BehaviourType, std::shared_ptr<EC_Lua_Script>> m_behaviours;
};

