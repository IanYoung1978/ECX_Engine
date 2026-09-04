#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "Engine/Subsystems/EC_System.h"
#include "Messaging/IEventListener.h"

// Forward declarations only - the Lua/luabridge headers and the ScriptAPI
// wrapper classes are only needed by the .cpp's method bodies, not by this
// class's own declaration. Keeping them out of the header means anything
// that just needs to know EC_LuaScriptSystem exists (most of the engine)
// doesn't have to recompile against ~1,200 lines of Lua binding machinery
// every time this file changes.
struct lua_State;
namespace ScriptAPI { struct GameAPI; }

class EC_LuaScriptSystem : public EC_System, public IEventListener {
public:
    EC_LuaScriptSystem();
    ~EC_LuaScriptSystem();

    void init(ECXMessenger& messenger, EC_Game& game) override;
    void update(const float& deltaTimeS, EC_Game& game) override;
    void receive(ECXEvent& event) override;

private:
    std::atomic<bool> m_shuttingDown{ false };
    lua_State* m_luaState;
    ScriptAPI::GameAPI* m_game;
    std::unordered_map<std::string, bool> m_loadedScripts;
    // Per-script-file handler functions, captured out of the shared global table right
    // after that file loads (see loadScript()) - every Lua script in the game executes
    // into ONE shared lua_State, so two different files both defining e.g. a global
    // `onKeyDown` would otherwise silently clobber each other (whichever loads second
    // wins, permanently). Keyed by script filename, then handler name (e.g. "onKeyDown");
    // value is a Lua registry reference (luaL_ref), not a luabridge::LuaRef, so this
    // header doesn't have to pull in the Lua/LuaBridge headers just to declare this map.
    std::unordered_map<std::string, std::unordered_map<std::string, int>> m_ScriptHandlers;
    std::mutex m_LuaMutex;

    const char* getEventFunctionName(ECXEventType type);
    bool loadScript(const std::string& filename);
    void callLuaFunction(const std::string& scriptFile, const char* funcName,
        EntityID entity, float deltaTime);
    void callLuaEvent(const std::string& scriptFile, const char* funcName,
        EntityID entity, ECXEvent& event);
    void registerAPI();
};
