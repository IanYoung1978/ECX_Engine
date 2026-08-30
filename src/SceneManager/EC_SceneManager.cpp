#include "EC_SceneManager.h"
#include "Game.h"
#include "xml/XML.h"
#include "Graphics/GL_Deferred_Renderer.h"
#include "Logging/ECX_Logging.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityFactory.h"
#include "Messaging/ECXMessenger.h"
#include "Messaging/ECXCommand.h"
#include "Messaging/ECXCommandType.h"
#include "UI/EC_UI_Factory.h"

namespace
{
    constexpr size_t kMaxGraphicsFinalizePerFrame = 8;
}

EC_SceneManager::EC_SceneManager()
{
}

EC_SceneManager::~EC_SceneManager()
{
}

void EC_SceneManager::init(EC_Game& game, std::string& config, ECXMessenger& messenger)
{
    m_Game = &game;

    if (!XML::loadGameModeSettings(config, m_Settings))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load scene manager settings: " + config,
            LOGGING::LogLevel::SEVERE);
        return;
    }

    m_Engine.init(m_Settings.engine_settings, game, messenger);

    EC_UI_Factory::loadUI(m_Settings.ui_file, messenger);
    EC_DOD_EntityFactory::loadManifestFile(m_Settings.physics_materials_file);

    messenger.Subscribe(*this, ECXCommandType::SystemStart);
    messenger.Subscribe(*this, ECXCommandType::SystemShutdown);
    messenger.Subscribe(*this, ECXCommandType::GamePause);
    messenger.Subscribe(*this, ECXCommandType::GameResume);

    RenderConfig renderConfig;
    if (!XML::loadRenderConfig(m_Settings.graphics_settings, renderConfig))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load graphics settings: " + m_Settings.graphics_settings + " - using defaults",
            LOGGING::LogLevel::WARNING);
    }

    m_Renderer = std::make_unique<GL_Deferred_Renderer>();
    m_Renderer->init(game.getWindow(), messenger, renderConfig);

    std::vector<XML::SceneDescriptor> descriptors;
    if (!XML::loadScenesFile(m_Settings.scenes_file, descriptors))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load scenes file: " + m_Settings.scenes_file,
            LOGGING::LogLevel::SEVERE);
        return;
    }

    // EC_GameScene owns a mutex (non-copyable/non-movable), and EC_DOD_LoadingWorker stores raw
    // EC_GameScene* pointers into m_Scenes, so this vector must never reallocate after scenes are
    // scheduled. Reserve the exact final size and construct in place.
    m_Scenes.reserve(descriptors.size());
    for (size_t i = 0; i < descriptors.size(); i++)
    {
        m_Scenes.emplace_back();
        EC_GameScene& scene = m_Scenes.back();
        scene.setAlias(descriptors[i].alias);
        scene.setFilename(descriptors[i].filename);
        scene.setPrecache(descriptors[i].precache);
        scene.setUnloadOnDeactivate(descriptors[i].unloadOnDeactivate);
        if (!descriptors[i].alias.empty())
            m_AliasMap[descriptors[i].alias] = i;
    }

    m_Loader = std::make_shared<EC_DOD_LoadingWorker>();
    m_ThreadManager.addTask(m_Loader);
    m_ThreadManager.executeTasks();

    for (size_t i = 0; i < m_Scenes.size(); i++)
    {
        if (m_Scenes[i].isPrecached())
        {
            m_Loader->scheduleScene(m_Scenes[i].getFilename(), m_Scenes[i]);
            m_LoadingScenes.insert(i);
        }
    }
    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Scheduling " + std::to_string(m_Scenes.size()) + " scenes, queue size before start: unknown",
        LOGGING::LogLevel::INFORMATION);
    m_Loader->start();
}

void EC_SceneManager::update(float deltaTimeS, EC_Game& game)
{
    // Resolve a handful of pending GPU resources every frame (not gated on the whole batch
    // finishing) so entities visibly pop in as they load, rather than appearing all at once.
    EC_DOD_EntityFactory::finalizePendingGraphics(kMaxGraphicsFinalizePerFrame);

    if (m_Loader->needsFinalization())
    {
        m_Loader->finalizeOnMainThread();

        for (auto it = m_LoadingScenes.begin(); it != m_LoadingScenes.end(); )
        {
            size_t idx = *it;
            if (m_Scenes[idx].isLoaded())
            {
                m_Renderer->bakeStaticShadows(m_Scenes[idx]);
                it = m_LoadingScenes.erase(it);
            }
            else
            {
                ++it;
            }
        }

        buildEntityMaps();
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Load complete!",
            LOGGING::LogLevel::INFORMATION);

        if (!m_InitialPauseDone)
        {
            // First scene load has finished - all its entities (and their
            // Transform components) now exist. Bake a correct initial
            // transform/camera state once, synchronously, then pause so
            // that first frame (rather than an identity-matrix/origin frame,
            // or several seconds of unwatched physics) is what's shown until
            // the player resumes.
            m_Engine.stepOnce(1.0f / 60.0f);
            m_Engine.pause();
            m_InitialPauseDone = true;
        }
    }

    m_Renderer->renderScene(m_Scenes[m_ActiveScene]);
}

void EC_SceneManager::loadScene(const std::string& alias)
{
    auto it = m_AliasMap.find(alias);
    if (it == m_AliasMap.end())
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "loadScene: unknown alias '" + alias + "'",
            LOGGING::LogLevel::SEVERE);
        return;
    }

    size_t idx = it->second;
    if (m_Scenes[idx].isLoaded() || m_LoadingScenes.count(idx))
        return;

    m_LoadingScenes.insert(idx);
    m_Loader->scheduleScene(m_Scenes[idx].getFilename(), m_Scenes[idx]);
    m_Loader->start();
}

void EC_SceneManager::unloadScene(const std::string& alias)
{
    auto it = m_AliasMap.find(alias);
    if (it == m_AliasMap.end())
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "unloadScene: unknown alias '" + alias + "'",
            LOGGING::LogLevel::SEVERE);
        return;
    }

    size_t idx = it->second;
    if (!m_Scenes[idx].isLoaded())
        return;

    if (idx == m_ActiveScene)
        m_Scenes[idx].deactivate();

    unloadSceneByIndex(idx);
}

void EC_SceneManager::activateScene(const std::string& alias)
{
    auto it = m_AliasMap.find(alias);
    if (it == m_AliasMap.end())
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "activateScene: unknown alias '" + alias + "'",
            LOGGING::LogLevel::SEVERE);
        return;
    }

    size_t idx = it->second;
    if (idx == m_ActiveScene)
        return;

    // Switch immediately rather than waiting for the load to finish: the renderer starts
    // drawing this scene's (initially near-empty, growing) entity list right away, and
    // entities become visible as the loader appends them and finalizePendingGraphics()
    // resolves their GPU resources over the following frames.
    if (!m_Scenes[idx].isLoaded())
        loadScene(alias);

    activateSceneByIndex(idx);
}

void EC_SceneManager::activateSceneByIndex(size_t index)
{
    size_t previous = m_ActiveScene;
    bool hadPrevious = previous < m_Scenes.size() && previous != index && m_Scenes[previous].isLoaded();

    if (hadPrevious)
    {
        m_Scenes[previous].deactivate();
        if (m_Scenes[previous].isUnloadOnDeactivate())
            unloadSceneByIndex(previous);
    }

    m_ActiveScene = index;
    m_Scenes[index].activate();
}

void EC_SceneManager::unloadSceneByIndex(size_t index)
{
    m_Scenes[index].unload();
    buildEntityMaps();
}

void EC_SceneManager::toggleDebug()
{
    m_Renderer->toggleDebug();
}

void EC_SceneManager::buildEntityMaps()
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    auto* infoArray = manager.getComponentArray<EC_DOD_EntityInfo>();
    if (!infoArray) return;

    m_UIDMap.clear();
    m_NameMap.clear();

    auto& data = infoArray->getData();
    for (size_t i = 0; i < data.size(); i++) {
        EntityID entity = infoArray->getEntity(i);
        m_UIDMap[data[i].uid] = entity;
        m_NameMap[data[i].name] = entity;
    }

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Entity maps built: " + std::to_string(m_UIDMap.size()) + " entities",
        LOGGING::LogLevel::INFORMATION);
}

EntityID EC_SceneManager::getEntityByUID(uint32_t uid) const
{
    auto it = m_UIDMap.find(uid);
    return (it != m_UIDMap.end()) ? it->second : INVALID_ENTITY;
}

EntityID EC_SceneManager::getEntityByName(const std::string& name) const
{
    auto it = m_NameMap.find(name);
    return (it != m_NameMap.end()) ? it->second : INVALID_ENTITY;
}

void EC_SceneManager::receive(ECXCommand& command)
{
    if (command.type == ECXCommandType::SystemShutdown)
    {
        m_Engine.pause();
        m_Engine.stop();
        m_Loader->shutdown();
    }
    if (command.type == ECXCommandType::SystemStart)
    {
        m_Engine.start();
    }
    if (command.type == ECXCommandType::GamePause)
    {
        m_Engine.pause();
    }
    if (command.type == ECXCommandType::GameResume)
    {
        m_Engine.resume();
    }
}
