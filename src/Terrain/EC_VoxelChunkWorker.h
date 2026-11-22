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

// Off-thread chunk mesh generation: a long-lived EC_Task on the shared pool looping on its own job
// queue, like EC_DOD_LoadingWorker. Chunks finish independently, so results go through a queue the
// main thread drains one at a time per frame.
class EC_VoxelChunkWorker : public EC_Task {
public:
    EC_VoxelChunkWorker();
    ~EC_VoxelChunkWorker();

    // Set once on the main thread before any chunk is scheduled (see EC_VoxelChunkSystem::init()).
    // EC_VolumeNode::evaluate() is stateless, so the worker can read this shared_ptr without locking once
    // set.
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
