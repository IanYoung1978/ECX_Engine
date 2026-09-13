#pragma once
#include <memory>
#include <vector>
#include "SceneManager/EC_SceneManager.h"
#include "Window/Window.h"
#include "Engine/Timer.h"
#include "Engine/Controllers/Controller.h"
#include "Engine/Subsystems/Control/ControlSystem.h"
#include "Messaging/ECXMessenger.h"
#include <mutex>
#include "TaskManager/EC_ThreadManager.h"
#include "Entity/EC_DOD_Types.h"
#include "Engine/Controllers/MouseButton.h"
#include "UI/EC_UI_InputSystem.h"
#include "Spatial/RayQueryHit.h"
#include "Terrain/EC_VoxelChunkSystem.h"
#include "Engine/Debug/EC_DebugHTTPServer.h"
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
    // Real capsule-vs-scene-geometry overlap query (see EC_BroadPhase::castCapsule) - the
    // capsule's own segment/radius against every candidate's actual collider, Mesh
    // (terrain) included via EC_CollisionChecks::CapsuleVsMesh. Unlike queryRay/queryCone,
    // this is a static overlap test, not a sweep - RayQueryHit::distance is repurposed as
    // penetration depth and ::normal points away from the capsule.
    // excludeEntity skips one candidate (INVALID_ENTITY = exclude nothing) - pass an
    // entity's own ID when it's querying its own capsule's surroundings, otherwise it
    // finds itself and reports a trivial full-penetration self-hit.
    std::vector<RayQueryHit> queryCapsule(const glm::vec3& pointA, const glm::vec3& pointB, float radius,
        bool firstHitOnly = false, uint32_t layerMask = 0xFFFFFFFFu, EntityID excludeEntity = INVALID_ENTITY);
    // Debug-draw the last ray/cone query (Issues #30/#29) - the single implementation
    // both EC_GameAPI's Lua bindings and the debug HTTP server's /rayQuery and /coneQuery
    // routes call into, so Lua and HTTP callers share one code path instead of each
    // publishing their own copy of the same command.
    void showDebugRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);
    void showDebugCone(const glm::vec3& apex, const glm::vec3& direction, float halfAngleDegrees, float maxDistance);
    // Requests a window resize (issue #108) - single code path shared by EC_GameAPI's Lua
    // setResolution() and the debug HTTP server's GET /resize, same pattern as
    // showDebugRay/showDebugCone above. Publishes SystemChangeResolution (an
    // already-declared but previously-unused ECXCommandType) rather than calling the window
    // directly, since this can be invoked from the scripting thread and SDL window calls
    // must happen on the main/GL thread - the renderer picks the command up via its own
    // receive() and calls Window::resize() from there. That triggers an OS resize event,
    // which Window::onResized()/EC_Game::run()'s event loop turns into the actual
    // GL_Deferred_Renderer::changeResolution() call once the new size is confirmed.
    void setResolution(int width, int height);
    // Same reasoning/pattern as setResolution() above - published as commands
    // (SystemToggleFullScreen/SystemMaximiseWindow/SystemMinimiseWindow, all previously
    // declared but unused) rather than called directly, since these are SDL window calls
    // that must happen on the main/GL thread and these methods may be invoked from Lua's
    // scripting thread via GameAPI.
    void toggleFullscreen();
    void maximizeWindow();
    void minimizeWindow();
    // Forwards to the scene manager's scripting system - see EC_LuaScriptSystem::
    // runScriptOnce/getVolumeRoot. Used by EC_VoxelChunkSystem::init() to run the active
    // game's terrain-generation script once and retrieve the shape it authored.
    bool runLuaScriptOnce(const std::string& filename);
    std::shared_ptr<EC_VolumeNode> getVolumeRoot() const;
    ScriptAPI::VoxelTerrainConfig getVoxelTerrainConfig() const;
    // Re-runs the terrain generation script and re-schedules every existing chunk against
    // the new shape, live - see EC_VoxelChunkSystem::regenerate(). Must be called from the
    // main thread (same requirement as everything else that touches m_VoxelChunkSystem).
    void regenerateTerrain();
    // Forwards to the scene manager's Audio system - see EC_AudioSystem.h. miniaudio's own
    // API is thread-safe, so unlike toggleFullscreen()/etc above these are safe to call
    // directly from GameAPI on the scripting thread without an ECXCommand-publish hop.
    void playSound(const std::string& path, float volume, const std::string& category);
    void playMusic(const std::string& path, float volume, bool loop);
    void stopMusic();
    void setCategoryVolume(const std::string& category, float volume);
    float getFPS() const { return m_Timer->getFPS(); }
    float getMSPF() const { return m_Timer->getMSPF(); }
    EntityID getEntityByUID(uint32_t uid) const;
    EntityID getEntityByName(const std::string& name) const;
    void toggleDebug();
    void loadScene(const std::string& alias);
    void unloadScene(const std::string& alias);
    void activateScene(const std::string& alias);
    bool isSceneActive(const std::string& alias) const;
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
    EC_VoxelChunkSystem m_VoxelChunkSystem;
    // Null unless EngineConfig.xml's <DebugHTTP enabled="true"> - shared_ptr because
    // EC_ThreadManager::addTask() requires it, matching EC_VoxelChunkWorker's ownership.
    std::shared_ptr<EC_DebugHTTPServer> m_DebugHTTPServer;
    std::mutex m_lock;
    void receive(ECXCommand& command) override;
};