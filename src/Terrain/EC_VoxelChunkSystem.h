#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "Engine/Subsystems/EC_System.h"
#include "TaskManager/EC_ThreadManager.h"
#include "Entity/EC_DOD_Types.h"

class EC_VoxelChunkWorker;
class Shader;

// The real, permanent voxel chunk integration - a fixed grid of chunk entities, each
// generated off the main thread (EC_VoxelChunkWorker) and uploaded on the main thread
// here. NOT wired into EC_PhysicsThreadTask's system list like Spatial/Transform/Camera/
// Scripting: those all run update() on the physics/background thread, but this system's
// update() does real GL work (ObjModel::initialiseFromMeshData) finishing each chunk, so
// it's called directly from EC_Game::update() instead - see that method for the call site,
// guaranteed main-thread since it also calls Window::present() right after.
class EC_VoxelChunkSystem : public EC_System {
public:
    // One voxel = one world unit; chunks are cubes of this size on a side. Referenced by
    // EC_VoxelChunkWorker when it builds each chunk's density field at coord * this.
    static constexpr int kChunkWorldSize = 32;

    EC_VoxelChunkSystem();
    virtual ~EC_VoxelChunkSystem();

    virtual void init(ECXMessenger& messenger, EC_Game& game) override;
    virtual void update(const float& deltaTimeS, EC_Game& game) override;

    // Issue #99 - must be called before init() if the caller wants a different generation
    // script than the default below. Can't be an init() parameter without changing every
    // EC_System's shared virtual signature, so this is a setter instead - EC_Game::init()
    // calls it (from EngineConfig.xml's <VoxelTerrain script="...">) only when voxel terrain
    // is actually enabled for this game.
    void setGenerationScriptPath(const std::string& path) { m_GenerationScriptPath = path; }

    // Must be called before EC_ThreadManager::stop() joins the shared pool - the worker's
    // execute() loop only exits once this notifies it, otherwise the join deadlocks on a
    // thread parked in the worker's own condvar wait.
    void shutdown();

    // Requests that terrain be regenerated from TerrainGeneration.lua, picked up on the
    // next update() tick (main thread only - see regenerate()'s own comment for why).
    // Thread-safe and cheap to call from anywhere: the debug HTTP server's
    // /regenerateTerrain route runs on an httplib worker thread, and
    // game:regenerateTerrain()'s Lua binding runs on the scripting subsystem's background
    // thread (EC_ScriptingTask) - neither is the main thread, so this only ever flips an
    // atomic flag rather than touching any ECS state itself.
    void requestRegenerate() { m_RegenerateRequested = true; }

private:
    void loadVolumeScript(EC_Game& game);
    // Re-runs the terrain generation script and re-schedules every existing chunk against
    // the new shape - lets an author iterate on TerrainGeneration.lua without a full engine
    // restart. Main-thread only: EC_DOD_EntityManager::getComponent() returns a raw
    // reference with no lock held through its use (unlike EC_BroadPhase's deliberately
    // snapshotted cross-thread reads), so mutating EC_DOD_VoxelChunk/collider/mesh
    // components from any other thread while update() concurrently reads/writes them on
    // the main thread would be a real data race. Only ever called from update(), which is
    // itself guaranteed main-thread (see this class's own top comment).
    void regenerate(EC_Game& game);

    std::atomic<bool> m_RegenerateRequested{ false };
    std::string m_GenerationScriptPath = "data/scripts/LUA/TerrainGeneration.lua";
    EC_ThreadManager m_ThreadManager;
    std::shared_ptr<EC_VoxelChunkWorker> m_Worker;
    std::shared_ptr<Shader> m_ChunkShader;
    // Chunks have no natural home in any EC_GameScene's own entity list (they're spawned
    // procedurally, not authored in a scene's XML), so scene switching can't filter them
    // the normal way (EC_GameScene::activate()/deactivate() only ever touches entities
    // explicitly added to it). Tracked here instead so update() can toggle their
    // EC_DOD_EntityInfo::sceneActive itself, keyed on whether "voxelchunkdemo" is the
    // active scene - without this, chunks stayed visible and collidable in every scene.
    std::vector<EntityID> m_ChunkEntities;
};
