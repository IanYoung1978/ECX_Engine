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
#include "Spatial/RayQueryHit.h"
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
    void pauseGame();
    void resumeGame();
    void update(const float& deltaTimeS);
    KeyState getKeyState(SDL_Scancode key);
    glm::ivec2 getMousePosition();
    void setMouseCaptured(bool captured);
    bool isMouseButtonPressed(MouseButton button);
    // Issue #30/#29. All entities the ray/cone intersects (not just the nearest) unless
    // firstHitOnly is set - the caller decides what matters. Synchronous, callable from
    // any C++ code with a game reference (physics callbacks, AI, renderer), mirroring the
    // ECXRequestType::FrustumCheck/EntitySearch pattern in EC_BroadPhase.
    std::vector<RayQueryHit> queryRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
        bool firstHitOnly = false, uint32_t layerMask = 0xFFFFFFFFu);
    // checkOcclusion opts into additionally requiring unobstructed line-of-sight to the
    // apex (excludes a candidate stacked behind a closer one) - independent of, not
    // fused with, containment: default false returns pure geometric containment.
    std::vector<RayQueryHit> queryCone(const glm::vec3& apex, const glm::vec3& direction, float halfAngleDegrees,
        float maxDistance, bool castsShadowOnly = true, bool checkOcclusion = false, uint32_t layerMask = 0xFFFFFFFFu);
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