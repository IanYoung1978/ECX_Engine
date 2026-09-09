#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <glm/glm.hpp>
#include "Entity/EC_DOD_EntityManager.h"
#include "TaskManager/EC_Task.h"
#include "Terrain/EC_TerrainMeshData.h"
#include "Procedural/EC_VolumeNode.h"

// Off-thread chunk mesh generation, shaped like EC_DOD_LoadingWorker (see that class for
// the precedent this mirrors): a long-lived EC_Task added once to the shared thread pool,
// whose execute() loops on its own job queue for the life of the program. Unlike the
// loading worker's single "ready to finalize" flag (suited to one batch finishing
// together), chunks complete independently and at different times, so results are
// reported through a queue the main thread drains one at a time each frame.
class EC_VoxelChunkWorker : public EC_Task {
public:
    EC_VoxelChunkWorker();
    ~EC_VoxelChunkWorker();

    // Set once on the main thread before any chunk is scheduled (see
    // EC_VoxelChunkSystem::init(), which runs the active game's generation script and
    // retrieves the resulting tree via EC_Game::getVolumeRoot() before scheduling any
    // chunks). EC_VolumeNode::evaluate() is pure/stateless, so reading this shared_ptr from
    // execute()'s worker thread with no additional locking is safe once set - the same
    // reasoning EC_Noise3D's use already relies on.
    void setVolumeRoot(EC_VolumeNodePtr root) { m_Root = std::move(root); }

    // Producer side (main thread): queue a chunk for generation.
    void scheduleChunk(const glm::ivec3& chunkCoord, EntityID entity);

    // Consumer side (main thread): pops one completed chunk's result if any are ready.
    // Returns false (leaving outEntity/outMesh untouched) if nothing has finished yet.
    bool tryDrainCompleted(EntityID& outEntity, EC_TerrainMeshData& outMesh);

    void shutdown();
    void execute() override;

private:
    struct ChunkJob {
        glm::ivec3 chunkCoord;
        EntityID entity;
    };
    struct CompletedChunk {
        EntityID entity;
        EC_TerrainMeshData meshData;
    };

    std::atomic<bool> m_Running;

    std::mutex m_QueueMutex;
    std::condition_variable m_QueueCV;
    std::deque<ChunkJob> m_JobQueue;

    std::mutex m_CompletedMutex;
    std::deque<CompletedChunk> m_CompletedQueue;

    EC_VolumeNodePtr m_Root;
};
