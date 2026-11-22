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
    // Returns false for an unknown alias instead of asserting or logging, so callers gating per-frame
    // behaviour on a scene don't need to special-case a typo.
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

    // Forwards to m_Engine's per-task tick-rate readout - see EC_Engine::getThreadRates().
    std::vector<EC_Engine::ThreadRate> getThreadRates() const;

    // Loads the prefab registered under alias, repositions it and registers it into the active scene.
    // Returns INVALID_ENTITY on any failure (unknown alias, file failed to load).
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
    // Written from game:activateScene() on the Scripting thread and read every frame on the main
    // thread, hence atomic.
    std::atomic<size_t> m_ActiveScene{ 0 };
    // activateSceneByIndex() only deactivates the old scene. Its entities are destroyed later by
    // sweepPendingDeletions() on the main thread, after the quarantine, so a script activating a scene
    // can't destroy the entity that is running it and no subsystem query can still hold a destroyed
    // entity. Configurable via <EntityLifecycle quarantineFrames>; default 30.
    uint64_t m_QuarantineFrames = 30;
    uint64_t m_FrameCounter = 0; // incremented once per update() call (main thread only)
    struct PendingSceneDeletion { size_t sceneIndex; uint64_t markedAtFrame; };
    std::mutex m_PendingDeletionsLock;
    std::vector<PendingSceneDeletion> m_PendingDeletions;
    void sweepPendingDeletions();
    std::unordered_map<std::string, size_t> m_AliasMap;
    // Rebuilt wholesale by buildEntityMaps() on the main thread after a scene loads, and read by
    // getEntityByUID()/getEntityByName(), which scripts call every frame from the scripting thread. An
    // unsynchronized read racing the rebuild is undefined behaviour, so a dedicated mutex guards every
    // access (as for m_LoadingScenes below).
    mutable std::mutex m_EntityMapsLock;
    std::unordered_map<uint32_t, EntityID> m_UIDMap;
    std::unordered_map<std::string, EntityID> m_NameMap;
    // Written from loadScene()/init() (reachable from Lua on the scripting thread) and iterated/erased
    // every frame on the main thread; an unsynchronized unordered_set is undefined behaviour, so a mutex
    // guards every access (as in EC_GameScene).
    std::mutex m_LoadingScenesLock;
    std::unordered_set<size_t> m_LoadingScenes;
    // alias -> standalone entity file path, loaded at init() from m_Settings.prefabs_file; empty if
    // absent.
    std::map<std::string, std::string> m_PrefabRegistry;
    EC_Game* m_Game = nullptr;
    std::mutex m_Lock;
    bool m_InitialPauseDone = false;
    // <Startup pauseOnStart="false"/> in EngineConfig.xml opts out of the auto-pause below -
    // see its own call site for why the pause exists at all. Defaults to true (existing
    // behaviour).
    bool m_PauseOnStart = true;
};
