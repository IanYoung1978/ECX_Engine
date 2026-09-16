#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <mutex>
#include <atomic>
#include "Engine/EC_Engine.h"
#include "Graphics/Renderers/Renderer.h"
#include "TaskManager/EC_DOD_LoadingWorker.h"
#include "TaskManager/EC_ThreadManager.h"
#include "Messaging/ICommandListener.h"
#include "Entity/EC_DOD_Types.h"
#include "SceneManager/EC_GameScene.h"
#include "Engine/GameModeSettings.h"

class EC_Game;
class ECXMessenger;
class EC_VolumeNode;

class EC_SceneManager : public ICommandListener
{
public:
    EC_SceneManager();
    ~EC_SceneManager();

    void init(EC_Game& game, std::string& config, ECXMessenger& messenger);
    void update(float deltaTimeS, EC_Game& game);
    void toggleDebug();

    EntityID getEntityByUID(uint32_t uid) const;
    EntityID getEntityByName(const std::string& name) const;

    void loadScene(const std::string& alias);
    void unloadScene(const std::string& alias);
    void activateScene(const std::string& alias);
    // Resolved path to EngineConfig.xml - lets EC_Game read its own additional sections
    // (e.g. <DebugHTTP>) without re-deriving this path itself or duplicating the
    // GameMode->EngineSettings resolution already done in init().
    const std::string& getEngineConfigPath() const { return m_Settings.engine_settings; }
    // Unknown alias returns false rather than asserting/logging - callers that just want to
    // gate their own per-frame behaviour on "is this particular scene the one showing right
    // now" (e.g. EC_VoxelChunkSystem tying its chunks' visibility to voxelchunkdemo) don't
    // need special-case handling for a typo'd or not-yet-registered alias.
    bool isSceneActive(const std::string& alias) const;
    // Forwards to the active Renderer - see Renderer::captureFrame's own comment. Must be
    // called from the GL/main thread.
    bool captureFrame(const std::string& target, std::vector<unsigned char>& outPNGBytes);
    // Forwards to the active Renderer's changeResolution (see GL_Deferred_Renderer.cpp) -
    // called from EC_Game::run()'s event loop once the window's own size has actually
    // changed (see Window::onResized). Must be called from the GL/main thread.
    void changeResolution(int width, int height);

    // Forwards to m_Engine's scripting system - see EC_LuaScriptSystem::runScriptOnce/
    // getVolumeRoot and EC_Engine's own forwarding methods.
    bool runLuaScriptOnce(const std::string& filename);
    std::shared_ptr<EC_VolumeNode> getVolumeRoot() const;
    ScriptAPI::VoxelTerrainConfig getVoxelTerrainConfig() const;

    // Forwards to m_Engine's Audio system - see EC_AudioSystem.h.
    void playSound(const std::string& path, float volume, const std::string& category);
    void playMusic(const std::string& path, float volume, bool loop);
    void stopMusic();
    void setCategoryVolume(const std::string& category, float volume);

    // Issue #130. Looks up alias in the prefab registry (see m_PrefabRegistry), loads that
    // standalone entity file, repositions it, and registers it into the currently active
    // scene - see EC_SceneManager.cpp's own comment for the full sequence. Returns
    // INVALID_ENTITY on any failure (unknown alias, file failed to load).
    EntityID spawnEntity(const std::string& alias, float x, float y, float z);
    // Thin wrapper over EC_DOD_EntityManager::destroyEntity - see that function's own
    // comment for why this doesn't also need to scrub the id out of whichever scene still
    // references it.
    void destroyEntity(EntityID id);

    void receive(ECXCommand& command) override;

private:
    void buildEntityMaps();
    void activateSceneByIndex(size_t index);
    void unloadSceneByIndex(size_t index);

    EC_Engine m_Engine;
    std::unique_ptr<Renderer> m_Renderer;
    std::shared_ptr<EC_DOD_LoadingWorker> m_Loader;
    EC_ThreadManager m_ThreadManager;
    GameModeSettings m_Settings;
    std::vector<EC_GameScene> m_Scenes;
    // Written from game:activateScene(), which Lua handlers call from the scripting
    // subsystem's thread (EC_ScriptingTask runs alongside physics on a background thread -
    // see EC_Engine::init()), and read every frame from update() on the main/render
    // thread. A plain size_t here was an unsynchronized cross-thread data race - the
    // render thread could observe a stale value, making a scene switch intermittently
    // fail to actually change what's drawn even though the switch itself succeeded.
    std::atomic<size_t> m_ActiveScene{ 0 };
    std::unordered_map<std::string, size_t> m_AliasMap;
    std::unordered_map<uint32_t, EntityID> m_UIDMap;
    std::unordered_map<std::string, EntityID> m_NameMap;
    // Issue #135 - written from loadScene()/init() (reachable from Lua's activateScene()
    // on the scripting thread) and iterated/erased every frame from update() on the main
    // thread. An unsynchronized insert concurrent with iterate/erase on a
    // std::unordered_set is undefined behaviour - observed as the engine hanging
    // entirely (a corrupted iterator or internal rehash mid-iteration, not a crash).
    // Same fix EC_GameScene already applies to its own m_Entities/m_Cameras/m_Lights: a
    // plain mutex around every access.
    std::mutex m_LoadingScenesLock;
    std::unordered_set<size_t> m_LoadingScenes;
    // Issue #130 - alias -> standalone entity file path, loaded once at init() from
    // m_Settings.prefabs_file (see XML::loadPrefabsFile). Empty if that file is absent.
    std::map<std::string, std::string> m_PrefabRegistry;
    EC_Game* m_Game = nullptr;
    std::mutex m_Lock;
    bool m_InitialPauseDone = false;
    // <Startup pauseOnStart="false"/> in EngineConfig.xml opts out of the auto-pause below -
    // see its own call site for why the pause exists at all. Defaults to true (existing
    // behaviour).
    bool m_PauseOnStart = true;
};
