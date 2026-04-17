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

    messenger.Subscribe(*this, ECXCommandType::SystemStart);
    messenger.Subscribe(*this, ECXCommandType::SystemShutdown);

    m_Renderer = std::make_unique<GL_Deferred_Renderer>();
    m_Renderer->init(game.getWindow(), messenger);

    std::vector<XML::SceneDescriptor> descriptors;
    if (!XML::loadScenesFile(m_Settings.scenes_file, descriptors))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Failed to load scenes file: " + m_Settings.scenes_file,
            LOGGING::LogLevel::SEVERE);
        return;
    }

    for (size_t i = 0; i < descriptors.size(); i++)
    {
        EC_GameScene scene;
        scene.setAlias(descriptors[i].alias);
        scene.setFilename(descriptors[i].filename);
        scene.setPrecache(descriptors[i].precache);
        scene.setUnloadOnDeactivate(descriptors[i].unloadOnDeactivate);
        if (!descriptors[i].alias.empty())
            m_AliasMap[descriptors[i].alias] = i;
        m_Scenes.push_back(scene);
    }

    m_Loader = std::make_shared<EC_DOD_LoadingWorker>();
    m_ThreadManager.addTask(m_Loader);
    m_ThreadManager.executeTasks();

    for (auto& scene : m_Scenes)
    {
        if (scene.isPrecached())
            m_Loader->scheduleScene(scene.getFilename(), scene);
    }
    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Scheduling " + std::to_string(m_Scenes.size()) + " scenes, queue size before start: unknown",
        LOGGING::LogLevel::INFORMATION);
    m_Loader->start();
}

void EC_SceneManager::update(float deltaTimeS, EC_Game& game)
{
    if (m_Loader->needsFinalization())
    {
        m_Loader->finalizeOnMainThread();
        m_Scenes[m_ActiveScene].activate();
        buildEntityMaps();
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Load complete!",
            LOGGING::LogLevel::INFORMATION);
    }

    if (m_Loader->isLoading())
    {
        float progress = m_Loader->getProgress();
    }

    m_Renderer->renderScene(m_Scenes[m_ActiveScene]);
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
}