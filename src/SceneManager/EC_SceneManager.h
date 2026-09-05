#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
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
    std::unordered_set<size_t> m_LoadingScenes;
    EC_Game* m_Game = nullptr;
    std::mutex m_Lock;
    bool m_InitialPauseDone = false;
};
