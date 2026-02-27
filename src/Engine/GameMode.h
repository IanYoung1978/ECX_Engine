#pragma once
#include "Window/Window.h"
#include "Subsystems/ControlSystem.h"
#include <memory>
#include <unordered_map>
#include <SDL.h>
#include "Graphics/Renderer.h"
#include "EC_Engine.h"
#include "GameModeSettings.h"
#include <deque>
#include "TaskManager/EC_DOD_LoadingWorker.h"
#include "TaskManager/EC_ThreadManager.h"
#include <mutex>
#include "Messaging/ICommandListener.h"
#include "Entity/EC_DOD_Types.h"

enum class Game_Mode
{
    Intro,
    Menu,
    Game,
    Credits,
    Num_Modes
};

class EC_Game;
class EC_Event;
class EC_Command;

class EC_GameMode : public ICommandListener
{
public:
    EC_GameMode();
    void init(EC_Game& game, std::string& config, ECXMessenger& messenger);
    void update(float deltaTimeS, EC_Game& game);
    void openMenu();
    void changeMode(Game_Mode mode);
    std::shared_ptr<Window> getWindow();
    ~EC_GameMode();

    EntityID getEntityByUID(uint32_t uid) const;
    EntityID getEntityByName(const std::string& name) const;

    void receive(ECXCommand& command) override;

private:
    void buildEntityMaps();

    EC_Game* m_game;
    GameModeSettings m_settings;
    std::vector<std::string> m_GameWorlds;
    EC_Engine m_engine;
    std::unique_ptr<Renderer> m_scene_renderer;
    std::deque<std::shared_ptr<EC_Event>> m_events;
    std::shared_ptr<EC_DOD_LoadingWorker> m_loader;
    EC_ThreadManager m_ThreadManager;
    std::mutex m_lock;

    std::unordered_map<uint32_t, EntityID> m_UIDMap;
    std::unordered_map<std::string, EntityID> m_NameMap;
};