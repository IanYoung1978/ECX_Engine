#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include "Engine/Subsystems/EC_System.h"
#include "Messaging/IEventListener.h"
#include "Engine/Subsystems/Scripting/EC_VolumeAPI.h"

// Forward declarations only: the Lua/luabridge headers and ScriptAPI classes are needed just by the
// .cpp, so the rest of the engine doesn't recompile when the bindings change.
struct lua_State;
namespace ScriptAPI { struct GameAPI; struct VolumeAPI; }
class EC_VolumeNode;

class EC_LuaScriptSystem : public EC_System, public IEventListener {
public:
    EC_LuaScriptSystem();
    ~EC_LuaScriptSystem();

    void init(ECXMessenger& messenger, EC_Game& game) override;
    void update(const float& deltaTimeS, EC_Game& game) override;
    void receive(ECXEvent& event) override;

    // Runs a script once, straight through, on the shared lua_State: unlike loadScript() it captures no
    // handlers. For one-shot setup scripts (e.g. terrain generation, see EC_VolumeAPI); may be called
    // again to re-run. Returns false on a Lua error (logged).
    bool runScriptOnce(const std::string& filename);

    // Valid only after runScriptOnce() on a script that called volume.setRoot(...) - null
    // otherwise. See EC_VolumeAPI::setRoot/getRoot.
    std::shared_ptr<EC_VolumeNode> getVolumeRoot() const;
    // Everything EC_VoxelChunkSystem needs besides the shape (see VoxelTerrainConfig); the struct's
    // defaults if no script has run.
    ScriptAPI::VoxelTerrainConfig getVoxelTerrainConfig() const;

private:
    std::atomic<bool> m_shuttingDown{ false };
    lua_State* m_luaState;
    ScriptAPI::GameAPI* m_game;
    ScriptAPI::VolumeAPI* m_volumeAPI;
    std::unordered_map<std::string, bool> m_loadedScripts;
    // Per-script-file handler functions, captured out of the shared global table right after each file
    // loads (see loadScript()): all scripts share one lua_State, so two files defining the same global
    // `onKeyDown` would clobber each other. Keyed by filename, then handler name; the value is a luaL_ref
    // registry reference so this header needn't include LuaBridge.
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
