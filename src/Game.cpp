#include "Game.h"
#include "Window/SDL_GL_Window.h"
#include "xml/XML.h"
#include "Engine/Keyboard.h"
#include "Engine/Config.h"
#include "Logging/ECX_Logging.h"
#include "Components/EC_DOD_Components.h"

EC_Game::EC_Game() : m_Running(true) {}

Game_Error EC_Game::init(const std::string& configurationFilename)
{
    GameSettings game_settings;
    LOGGING::ECX_Logger::GetInstance()->setOutputFilename("log.html");
    if (!XML::loadGameConfig(configurationFilename, game_settings))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("Config error", LOGGING::LogLevel::CRITICAL);
        return Game_Error::CONFIG_ERROR;
    }

    WindowSettings settings;
    if (!XML::createWindowSettings(game_settings.window_settings_file, settings))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("Config error", LOGGING::LogLevel::CRITICAL);
        return Game_Error::CONFIG_ERROR;
    }

    m_Window = std::make_shared<SDL_GL_Window>();
    if (!m_Window->init(settings))
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage("Window error", LOGGING::LogLevel::CRITICAL);
        return Game_Error::WINDOW_ERROR;
    }

    m_Controls = std::make_shared<ControlSystem>();
    m_Controls->init(m_Messenger, *this);

    for (auto& s : game_settings.GameModes)
        m_SceneManager.init(*this, s, m_Messenger);

    m_Timer = std::make_unique<Timer>();
    m_threadmanager.init(8);
    m_Running = true;
    m_Messenger.Subscribe(*this, ECXCommandType::SystemShutdown);
    LOGGING::ECX_Logger::GetInstance()->LogMessage("Init complete", LOGGING::LogLevel::INFORMATION);
    return Game_Error::NO_ERROR;
}

Game_Error EC_Game::run()
{
    SDL_Event e;
    ECXCommand command;
    command.type = ECXCommandType::SystemStart;
    m_Messenger.publish(command);

    while (m_Running)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                m_Running = false;
            m_Controls->handleEvent(e);
        }
        m_Timer->update(*this);
    }

    m_Window->close();
    LOGGING::ECX_Logger::GetInstance()->LogMessage("Shutting down", LOGGING::LogLevel::INFORMATION);
    LOGGING::ECX_Logger::GetInstance()->printToFile();
    return Game_Error::NO_ERROR;
}

EntityID EC_Game::getEntityByUID(uint32_t uid) const
{
    return m_SceneManager.getEntityByUID(uid);
}

EntityID EC_Game::getEntityByName(const std::string& name) const
{
    return m_SceneManager.getEntityByName(name);
}

void EC_Game::toggleDebug()
{
    m_SceneManager.toggleDebug();
}

void EC_Game::loadScene(const std::string& alias)
{
    m_SceneManager.loadScene(alias);
}

void EC_Game::unloadScene(const std::string& alias)
{
    m_SceneManager.unloadScene(alias);
}

void EC_Game::activateScene(const std::string& alias)
{
    m_SceneManager.activateScene(alias);
}

void EC_Game::setStreamingReferencePosition(const glm::vec3& position)
{
    m_SceneManager.setStreamingReferencePosition(position);
}

void EC_Game::shutDown()
{
    ECXCommand command;
    command.type = ECXCommandType::SystemShutdown;
    m_Messenger.publish(command);
}

void EC_Game::update(const float& deltaTimeS)
{
    if (m_Running)
    {
        m_Messenger.flush();
        m_SceneManager.update(deltaTimeS, *this);
        m_Controls->update(deltaTimeS, *this);
        m_Window->present();
    }
}

KeyState EC_Game::getKeyState(SDL_Scancode key)
{
    return m_Controls->getKeyState(key);
}

std::shared_ptr<Window> EC_Game::getWindow()
{
    return m_Window;
}

std::shared_ptr<ControlSystem> EC_Game::getControls()
{
    return m_Controls;
}

void EC_Game::receive(ECXCommand& command)
{
    if (command.type == ECXCommandType::SystemShutdown)
    {
        m_Running = false;
        m_Controls->shutdown();
        m_threadmanager.stop();
    }
}

EC_Game::~EC_Game() {}