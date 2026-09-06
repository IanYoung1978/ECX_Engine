#include "Terrain/EC_VoxelChunkWorker.h"
#include "Terrain/EC_DensityField.h"
#include "Terrain/EC_MarchingCubesMesher.h"
#include "Terrain/EC_TerrainWorldDensity.h"
#include "Terrain/EC_VoxelChunkSystem.h"

EC_VoxelChunkWorker::EC_VoxelChunkWorker()
    : m_Running(true)
{
}

EC_VoxelChunkWorker::~EC_VoxelChunkWorker() {
    shutdown();
}

void EC_VoxelChunkWorker::scheduleChunk(const glm::ivec3& chunkCoord, EntityID entity) {
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_JobQueue.push_back({ chunkCoord, entity });
    }
    m_QueueCV.notify_one();
}

void EC_VoxelChunkWorker::shutdown() {
    m_Running = false;
    {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_JobQueue.clear();
    }
    m_QueueCV.notify_one();
}

bool EC_VoxelChunkWorker::tryDrainCompleted(EntityID& outEntity, EC_TerrainMeshData& outMesh) {
    std::lock_guard<std::mutex> lock(m_CompletedMutex);
    if (m_CompletedQueue.empty()) return false;

    CompletedChunk& front = m_CompletedQueue.front();
    outEntity = front.entity;
    outMesh = std::move(front.meshData);
    m_CompletedQueue.pop_front();
    return true;
}

void EC_VoxelChunkWorker::execute() {
    while (m_Running) {
        ChunkJob job;
        bool hasJob = false;
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_QueueCV.wait(lock, [this] { return !m_Running || !m_JobQueue.empty(); });
            if (!m_Running) break;
            job = m_JobQueue.front();
            m_JobQueue.pop_front();
            hasJob = true;
        }
        if (!hasJob) continue;

        glm::vec3 chunkOrigin(
            static_cast<float>(job.chunkCoord.x) * EC_VoxelChunkSystem::kChunkWorldSize,
            static_cast<float>(job.chunkCoord.y) * EC_VoxelChunkSystem::kChunkWorldSize,
            static_cast<float>(job.chunkCoord.z) * EC_VoxelChunkSystem::kChunkWorldSize);

        EC_DensityField field(EC_VoxelChunkSystem::kChunkWorldSize);
        int size = field.interiorSize;
        for (int z = -EC_DensityField::Padding; z < size + EC_DensityField::Padding; z++) {
            for (int y = -EC_DensityField::Padding; y < size + EC_DensityField::Padding; y++) {
                for (int x = -EC_DensityField::Padding; x < size + EC_DensityField::Padding; x++) {
                    glm::vec3 worldPos = chunkOrigin + glm::vec3(
                        static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    field.at(x, y, z) = EC_TerrainWorldDensity::sampleWorldDensity(worldPos);
                }
            }
        }

        EC_TerrainMeshData meshData = EC_MarchingCubesMesher::polygonise(field, 0.0f);

        {
            std::lock_guard<std::mutex> lock(m_CompletedMutex);
            m_CompletedQueue.push_back({ job.entity, std::move(meshData) });
        }
    }
}
