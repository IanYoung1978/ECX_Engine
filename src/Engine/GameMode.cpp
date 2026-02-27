#include "GameMode.h"
#include "Game.h"
#include "Graphics/GL_Deferred_Renderer.h"
#include "xml/XML.h"
#include "Logging/ECX_Logging.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"

EC_GameMode::EC_GameMode()
{
    m_game = nullptr;
}

void EC_GameMode::init(EC_Game& game, std::string& config, ECXMessenger& messenger)
{
    if (XML::loadGameModeSettings(config, m_settings))
    {
        m_engine.init(m_settings.engine_settings, game, messenger);
        m_game = &game;
    }
    messenger.Subscribe(*this, ECXCommandType::SystemStart);
    messenger.Subscribe(*this, ECXCommandType::SystemShutdown);
    m_scene_renderer = std::make_unique<GL_Deferred_Renderer>();
    m_scene_renderer->init(game.getWindow());
    m_loader = std::make_shared<EC_DOD_LoadingWorker>();
    m_ThreadManager.addTask(m_loader);
    m_ThreadManager.executeTasks();
    m_loader->scheduleScene(m_settings.game_world_data);
    m_loader->start();
}

void EC_GameMode::update(float deltaTimeS, EC_Game& game)
{
    if (m_loader->needsFinalization())
    {
        m_loader->finalizeOnMainThread();
        buildEntityMaps();
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Load complete!",
            LOGGING::LogLevel::INFORMATION
        );
    }

    if (m_loader->isLoading())
    {
        float progress = m_loader->getProgress();
    }

    m_scene_renderer->renderScene();
}

void EC_GameMode::buildEntityMaps()
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    auto* infoArray = manager.getComponentArray<EC_DOD_EntityInfo>();
    if (!infoArray) return;

    // Rebuild both maps from scratch on each load
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
        LOGGING::LogLevel::INFORMATION
    );
}

EntityID EC_GameMode::getEntityByUID(uint32_t uid) const
{
    auto it = m_UIDMap.find(uid);
    if (it != m_UIDMap.end()) return it->second;
    return INVALID_ENTITY;
}

EntityID EC_GameMode::getEntityByName(const std::string& name) const
{
    auto it = m_NameMap.find(name);
    if (it != m_NameMap.end()) return it->second;
    return INVALID_ENTITY;
}

void EC_GameMode::openMenu()
{
}

EC_GameMode::~EC_GameMode()
{
}

void EC_GameMode::receive(ECXCommand& command)
{
    if (command.type == ECXCommandType::SystemShutdown)
    {
        m_engine.pause();
        m_engine.stop();
        m_loader->shutdown();
    }
    if (command.type == ECXCommandType::SystemStart)
    {
        m_engine.start();
    }
}

void EC_GameMode::changeMode(Game_Mode mode)
{
    m_game->changeMode(mode);
}

std::shared_ptr<Window> EC_GameMode::getWindow()
{
    return m_game->getWindow();
}