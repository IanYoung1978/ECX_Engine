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
    // All entities the ray/cone intersects unless firstHitOnly is set. Synchronous and callable from
    // any C++ code with a game reference.
    std::vector<RayQueryHit> queryRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
        bool firstHitOnly = false, uint32_t layerMask = 0xFFFFFFFFu);
    // checkOcclusion opts into additionally requiring unobstructed line-of-sight to the
    // apex (excludes a candidate stacked behind a closer one) - independent of, not
    // fused with, containment: default false returns pure geometric containment.
    std::vector<RayQueryHit> queryCone(const glm::vec3& apex, const glm::vec3& direction, float halfAngleDegrees,
        float maxDistance, bool castsShadowOnly = true, bool checkOcclusion = false, uint32_t layerMask = 0xFFFFFFFFu);
    // Capsule-vs-scene overlap (see EC_BroadPhase::castCapsule), including Mesh terrain. A static
    // overlap test, not a sweep: RayQueryHit::distance is penetration depth and normal points away from
    // the capsule. excludeEntity skips one candidate (INVALID_ENTITY = none); pass the caller's own ID or
    // it finds itself.
    std::vector<RayQueryHit> queryCapsule(const glm::vec3& pointA, const glm::vec3& pointB, float radius,
        bool firstHitOnly = false, uint32_t layerMask = 0xFFFFFFFFu, EntityID excludeEntity = INVALID_ENTITY);
    // Debug-draw the last ray/cone query; the single implementation behind both the Lua bindings and
    // the debug HTTP routes.
    void showDebugRay(const glm::vec3& origin, const glm::vec3& direction, float maxDistance);
    void showDebugCone(const glm::vec3& apex, const glm::vec3& direction, float halfAngleDegrees, float maxDistance);
    // Requests a window resize, shared by Lua setResolution() and GET /resize. Published as a command
    // because SDL window calls must happen on the main/GL thread and this can be called from the
    // scripting thread.
    void setResolution(int width, int height);
    // Same as setResolution(): published as commands, since these are SDL window calls that must run
    // on the main/GL thread.
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
    // Forwards to the scene manager's engine - see EC_Engine::getThreadRates(). Main
    // thread's own rate is getFPS() above, not included here.
    std::vector<EC_Engine::ThreadRate> getThreadRates() const;
    // Forwards to the scene manager's prefab registry (see EC_SceneManager::spawnEntity/destroyEntity).
    EntityID spawnEntity(const std::string& alias, float x, float y, float z);
    void destroyEntity(EntityID id);
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