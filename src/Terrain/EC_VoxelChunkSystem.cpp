#include "Terrain/EC_VoxelChunkSystem.h"
#include "Terrain/EC_VoxelChunkWorker.h"
#include "Terrain/EC_TerrainMeshData.h"
#include "Components/EC_DOD_Components.h"
#include "Components/EC_CollisionLayers.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Graphics/Models/ObjModel.h"
#include "Graphics/Shaders/Shader.h"
#include "Logging/ECX_Logging.h"

namespace {
    // Small fixed grid, one Y layer - enough to prove multiple chunks stitch together
    // seamlessly without a distance-based streaming trigger yet (deferred to a later
    // slice, along with LOD).
    constexpr int kGridRadius = 1; // chunks from -1..1 in X and Z => 3x3

    // Chunks finishing several at once (e.g. right after startup) would spike a frame if
    // all uploaded in one go - matches the throttling rationale behind
    // EC_DOD_EntityFactory::finalizePendingGraphics(maxPerCall).
    constexpr int kMaxUploadsPerFrame = 2;
}

EC_VoxelChunkSystem::EC_VoxelChunkSystem() {
}

EC_VoxelChunkSystem::~EC_VoxelChunkSystem() {
    shutdown();
}

void EC_VoxelChunkSystem::shutdown() {
    if (m_Worker) m_Worker->shutdown();
}

void EC_VoxelChunkSystem::init(ECXMessenger& messenger, EC_Game& game) {
    m_ChunkShader = std::make_shared<Shader>();
    if (!m_ChunkShader->loadShader("data/assets/shaders/basic.vert", "data/assets/shaders/PBR.frag")) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_VoxelChunkSystem: failed to load chunk shader", LOGGING::LogLevel::CRITICAL);
        return;
    }

    m_Worker = std::make_shared<EC_VoxelChunkWorker>();
    m_ThreadManager.addTask(m_Worker);
    m_ThreadManager.executeTasks();

    auto& manager = EC_DOD_EntityManager::getInstance();

    for (int x = -kGridRadius; x <= kGridRadius; x++) {
        for (int z = -kGridRadius; z <= kGridRadius; z++) {
            glm::ivec3 coord(x, 0, z);

            EntityID entity = manager.createEntity();

            EC_DOD_Spatial spatial;
            spatial.position = glm::vec3(coord) * static_cast<float>(kChunkWorldSize);
            manager.addComponent(entity, spatial);

            EC_DOD_Transform transform;
            manager.addComponent(entity, transform);

            EC_DOD_GraphicsData gfx;
            gfx.shader = m_ChunkShader;
            gfx.colour = glm::vec4(0.5f, 0.45f, 0.35f, 1.0f);
            manager.addComponent(entity, gfx);

            EC_DOD_VoxelChunk chunk;
            chunk.chunkCoord = coord;
            chunk.state = EC_DOD_VoxelChunk::State::Generating;
            manager.addComponent(entity, chunk);

            m_Worker->scheduleChunk(coord, entity);
        }
    }
}

void EC_VoxelChunkSystem::update(const float& deltaTimeS, EC_Game& game) {
    if (!m_Worker) return;

    auto& manager = EC_DOD_EntityManager::getInstance();

    for (int i = 0; i < kMaxUploadsPerFrame; i++) {
        EntityID entity;
        EC_TerrainMeshData meshData;
        if (!m_Worker->tryDrainCompleted(entity, meshData)) break;

        if (!manager.isAlive(entity)) continue;
        if (!manager.hasComponent<EC_DOD_GraphicsData>(entity)) continue;
        if (!manager.hasComponent<EC_DOD_VoxelChunk>(entity)) continue;
        if (meshData.positions.empty()) continue;

        auto model = std::make_shared<ObjModel>();
        model->initialiseFromMeshData(meshData);

        auto& gfx = manager.getComponent<EC_DOD_GraphicsData>(entity);
        gfx.model = model;

        // Renderable so EC_BroadPhase's spatial index (and thus frustum culling) can find
        // this entity at all - constructEntity() does this automatically for XML-authored
        // entities, but chunks bypass that path entirely. Left collidable (default mask)
        // rather than mask=0 (render-only) since a chunk is meant to be a real, solid
        // piece of ground once gameplay collision exists - not yet exercised by the
        // current free-fly debug camera, which has no physics body of its own.
        EC_DOD_Collider collider;
        collider.type = EC_DOD_Collider::Type::AABB;
        float half = static_cast<float>(kChunkWorldSize) * 0.5f;
        collider.extents = glm::vec3(half);
        collider.center = glm::vec3(half);
        collider.collisionLayer |= CollisionLayers::Renderable;
        manager.addComponent(entity, collider);

        manager.getComponent<EC_DOD_VoxelChunk>(entity).state = EC_DOD_VoxelChunk::State::Ready;

        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_VoxelChunkSystem: chunk ready (" + std::to_string(meshData.positions.size()) + " vertices)",
            LOGGING::LogLevel::INFORMATION);
    }
}
