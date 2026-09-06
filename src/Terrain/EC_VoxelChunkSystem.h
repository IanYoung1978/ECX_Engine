#pragma once
#include <memory>
#include "Engine/Subsystems/EC_System.h"
#include "TaskManager/EC_ThreadManager.h"

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

    // Must be called before EC_ThreadManager::stop() joins the shared pool - the worker's
    // execute() loop only exits once this notifies it, otherwise the join deadlocks on a
    // thread parked in the worker's own condvar wait.
    void shutdown();

private:
    EC_ThreadManager m_ThreadManager;
    std::shared_ptr<EC_VoxelChunkWorker> m_Worker;
    std::shared_ptr<Shader> m_ChunkShader;
};
