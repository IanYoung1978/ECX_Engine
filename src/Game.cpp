#include "Game.h"
#include "Window/SDL_GL_Window.h"
#include "xml/XML.h"
#include "Engine/Controllers/Keyboard.h"
#include "Engine/Config.h"
#include "Logging/ECX_Logging.h"
#include "Components/EC_DOD_Components.h"
#include "Messaging/ECXRequest.h"
#include "Messaging/ECXResponse.h"
#include "Messaging/ECXRequestType.h"
#include <cmath>

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

void EC_Game::shutDown()
{
    ECXCommand command;
    command.type = ECXCommandType::SystemShutdown;
    m_Messenger.publish(command);
}

void EC_Game::pauseGame()
{
    ECXCommand command;
    command.type = ECXCommandType::GamePause;
    m_Messenger.publish(command);
}

void EC_Game::resumeGame()
{
    ECXCommand command;
    command.type = ECXCommandType::GameResume;
    m_Messenger.publish(command);
}

void EC_Game::update(const float& deltaTimeS)
{
    if (m_Running)
    {
        m_Messenger.flush();
        m_SceneManager.update(deltaTimeS, *this);
        m_Controls->update(deltaTimeS, *this);
        m_UIInput.update(*this, m_Messenger);
        m_Window->present();
    }
}

KeyState EC_Game::getKeyState(SDL_Scancode key)
{
    return m_Controls->getKeyState(key);
}

glm::ivec2 EC_Game::getMousePosition()
{
    return m_Controls->getMouse()->getMousePosition();
}

void EC_Game::setMouseCaptured(bool captured)
{
    m_Controls->getMouse()->setFPSMode(captured);
}

bool EC_Game::isMouseButtonPressed(MouseButton button)
{
    return m_Controls->getMouse()->keyPressed(button);
}

std::vector<RayQueryHit> EC_Game::queryRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
    bool firstHitOnly, uint32_t layerMask)
{
    ECXRequest request;
    request.type = ECXRequestType::RayCheck;
    request.args[0] = origin;
    request.args[1] = direction;
    request.args[2] = maxDistance;
    request.args[3] = layerMask;
    request.args[4] = firstHitOnly;

    ECXResponse response;
    m_Messenger.publish(request, response);

    if (response.response != ECXResponseType::Success || response.responseData.empty())
        return {};

    try {
        return std::any_cast<std::vector<RayQueryHit>>(response.responseData[0]);
    }
    catch (const std::bad_any_cast&) {
        return {};
    }
}

std::vector<RayQueryHit> EC_Game::queryCone(const glm::vec3& apex, const glm::vec3& direction, float halfAngleDegrees,
    float maxDistance, bool castsShadowOnly, bool checkOcclusion, uint32_t layerMask)
{
    ECXRequest request;
    request.type = ECXRequestType::ConeCheck;
    request.args[0] = apex;
    request.args[1] = direction;
    request.args[2] = glm::radians(halfAngleDegrees);
    request.args[3] = maxDistance;
    request.args[4] = layerMask;
    request.args[5] = castsShadowOnly;
    request.args[6] = checkOcclusion;

    ECXResponse response;
    m_Messenger.publish(request, response);

    if (response.response != ECXResponseType::Success || response.responseData.empty())
        return {};

    try {
        return std::any_cast<std::vector<RayQueryHit>>(response.responseData[0]);
    }
    catch (const std::bad_any_cast&) {
        return {};
    }
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