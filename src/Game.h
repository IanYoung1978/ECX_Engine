#pragma once
#include <memory>
#include <vector>
#include "SceneManager/EC_SceneManager.h"
#include "Window/Window.h"
#include "Engine/Timer.h"
#include "Engine/Controller.h"
#include "Engine/Subsystems/ControlSystem.h"
#include "Messaging/ECXMessenger.h"
#include <mutex>
#include "TaskManager/EC_ThreadManager.h"
#include "Entity/EC_DOD_Types.h"
#include "Engine/MouseButton.h"
#include "UI/EC_UI_InputSystem.h"
#include <glm/glm.hpp>

enum class Game_Error
{
    NO_ERROR,
    CONFIG_ERROR,
    WINDOW_ERROR,
    NUM_ERRORS
};

class EC_Game : public ICommandListener
{
public:
    EC_Game();
    Game_Error init(const std::string& configurationFilename);
    Game_Error run();
    void shutDown();
    void update(const float& deltaTimeS);
    KeyState getKeyState(SDL_Scancode key);
    glm::ivec2 getMousePosition();
    void setMouseCaptured(bool captured);
    bool isMouseButtonPressed(MouseButton button);
    float getFPS() const { return m_Timer->getFPS(); }
    float getMSPF() const { return m_Timer->getMSPF(); }
    EntityID getEntityByUID(uint32_t uid) const;
    EntityID getEntityByName(const std::string& name) const;
    void toggleDebug();
    void loadScene(const std::string& alias);
    void unloadScene(const std::string& alias);
    void activateScene(const std::string& alias);
    std::shared_ptr<Window> getWindow();
    ~EC_Game();

private:
    ECXMessenger m_Messenger;
    std::shared_ptr<ControlSystem> getControls();
    EC_SceneManager m_SceneManager;
    std::shared_ptr<Window> m_Window;
    std::unique_ptr<Timer> m_Timer;
    std::shared_ptr<ControlSystem> m_Controls;
    bool m_Running;
    EC_UI_InputSystem m_UIInput;
    EC_ThreadManager m_threadmanager;
    std::mutex m_lock;
    void receive(ECXCommand& command) override;
};