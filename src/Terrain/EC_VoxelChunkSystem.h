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

// Fixed grid of chunk entities, each generated off the main thread (EC_VoxelChunkWorker) and
// uploaded here. Not on EC_PhysicsThreadTask: update() does GL work
// (ObjModel::initialiseFromMeshData), so EC_Game::update() calls it directly on the main thread.
class EC_VoxelChunkSystem : public EC_System {
public:
    // One voxel = one world unit; chunks are cubes of this size on a side. Referenced by
    // EC_VoxelChunkWorker when it builds each chunk's density field at coord * this.
    static constexpr int kChunkWorldSize = 32;

    EC_VoxelChunkSystem();
    virtual ~EC_VoxelChunkSystem();

    virtual void init(ECXMessenger& messenger, EC_Game& game) override;
    virtual void update(const float& deltaTimeS, EC_Game& game) override;

    // Set before init() to use a different generation script than the default. A setter rather than
    // an init() parameter because init() is EC_System's shared signature.
    void setGenerationScriptPath(const std::string& path) { m_GenerationScriptPath = path; }

    // Must be called before EC_ThreadManager::stop() joins the shared pool - the worker's
    // execute() loop only exits once this notifies it, otherwise the join deadlocks on a
    // thread parked in the worker's own condvar wait.
    void shutdown();

    // Callable from any thread; only sets a flag, the regeneration runs on the main thread's next update().
    void requestRegenerate() { m_RegenerateRequested = true; }

private:
    void loadVolumeScript(EC_Game& game);
    // Re-runs the terrain generation script and reschedules every chunk. Main-thread only:
    // getComponent() returns a raw reference with no lock held, so mutating chunk components from
    // another thread while update() runs would race. Only called from update().
    void regenerate(EC_Game& game);

    std::atomic<bool> m_RegenerateRequested{ false };
    std::string m_GenerationScriptPath = "data/scripts/LUA/TerrainGeneration.lua";
    EC_ThreadManager m_ThreadManager;
    std::shared_ptr<EC_VoxelChunkWorker> m_Worker;
    std::shared_ptr<Shader> m_ChunkShader;
    // Chunks aren't in any EC_GameScene's entity list, so they're tracked here and update() toggles
    // their EC_DOD_EntityInfo::sceneState.
    std::vector<EntityID> m_ChunkEntities;
};
