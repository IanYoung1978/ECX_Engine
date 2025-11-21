#pragma once
#include "IComponent.h"
#include <string>
#include <unordered_map>


class EC_ScriptComponent : public IComponent {
public:
    EC_ScriptComponent(const std::string& scriptFile = "")
        : m_scriptFile(scriptFile), enabled(true) {
    }

    virtual ~EC_ScriptComponent() = default;

    // Script file path
    const std::string& getScriptFile() const { return m_scriptFile; }
    void setScriptFile(const std::string& file) { m_scriptFile = file; }

    // Enable/disable without removing component
    bool isEnabled() const { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }

    // Per-entity persistent variables (accessible from Lua)
    // These survive between frames - perfect for entity state
    std::unordered_map<std::string, float> floatVars;
    std::unordered_map<std::string, std::string> stringVars;

private:
    std::string m_scriptFile;
    bool enabled;

    // Inherited via IComponent
    ComponentType getComponentType() override;
};

/*
USAGE EXAMPLES:

// C++: Create scripted entity
auto entity = std::make_shared<GameEntity>();
entity->setName("Player");

auto script = std::make_shared<ScriptComponent>("scripts/player.lua");
entity->addComponent(std::type_index(typeid(ScriptComponent)), script);

// C++: Initialize script variables (optional)
script->floatVars["health"] = 100.0f;
script->floatVars["maxHealth"] = 100.0f;
script->stringVars["weapon"] = "sword";

// C++: Disable script temporarily
script->disable();

// Lua: Access these variables
-- function update(entity, deltaTime)
--     local health = entity:getFloat("health", 100.0)
--     entity:setFloat("health", health - 1.0)
-- end
*/