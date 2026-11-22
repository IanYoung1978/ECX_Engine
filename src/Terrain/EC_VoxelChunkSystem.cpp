#include "Terrain/EC_VoxelChunkSystem.h"
#include "Terrain/EC_VoxelChunkWorker.h"
#include "Game.h"
#include "Terrain/EC_TerrainMeshData.h"
#include "Components/EC_DOD_Components.h"
#include "Components/EC_CollisionLayers.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Graphics/Models/ObjModel.h"
#include "Graphics/Shaders/Shader.h"
#include "Logging/ECX_Logging.h"
#include "Engine/Subsystems/Scripting/EC_VolumeAPI.h"

namespace {
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
    m_Worker = std::make_shared<EC_VoxelChunkWorker>();
    // Run the generation script first: besides the shape (setRoot) it can configure chunk material,
    // tint and grid size via volume.setChunkMaterial()/setChunkColour()/setGridRadius() (see
    // VoxelTerrainConfig for the defaults).
    loadVolumeScript(game);
    ScriptAPI::VoxelTerrainConfig config = game.getVoxelTerrainConfig();

    m_ChunkShader = std::make_shared<Shader>();
    if (!m_ChunkShader->loadShader(config.chunkVertShader, config.chunkFragShader)) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_VoxelChunkSystem: failed to load chunk shader", LOGGING::LogLevel::CRITICAL);
        return;
    }

    m_ThreadManager.addTask(m_Worker);
    m_ThreadManager.executeTasks();

    auto& manager = EC_DOD_EntityManager::getInstance();

    // One Y layer, radius from config (was a fixed 3x3 - kGridRadius=1 - just enough to
    // prove multiple chunks stitch together seamlessly; distance-based streaming and LOD
    // are still a later slice regardless of grid size).
    for (int x = -config.gridRadius; x <= config.gridRadius; x++) {
        for (int z = -config.gridRadius; z <= config.gridRadius; z++) {
            glm::ivec3 coord(x, 0, z);

            EntityID entity = manager.createEntity();

            EC_DOD_Spatial spatial;
            spatial.position = glm::vec3(coord) * static_cast<float>(kChunkWorldSize);
            manager.addComponent(entity, spatial);

            EC_DOD_Transform transform;
            manager.addComponent(entity, transform);

            EC_DOD_GraphicsData gfx;
            gfx.shader = m_ChunkShader;
            gfx.colour = config.chunkColour;
            manager.addComponent(entity, gfx);

            EC_DOD_VoxelChunk chunk;
            chunk.chunkCoord = coord;
            chunk.state = EC_DOD_VoxelChunk::State::Generating;
            manager.addComponent(entity, chunk);

            // Chunks aren't in any EC_GameScene's entity list, so scene switching never touches them; this
            // component lets update() set their sceneState. Without it a component-less entity is treated as
            // always active.
            EC_DOD_EntityInfo info;
            info.name = "voxelchunk_" + std::to_string(x) + "_" + std::to_string(z);
            info.active = true;
            info.sceneState = EC_SceneLifecycleState::Active;
            manager.addComponent(entity, info);

            m_ChunkEntities.push_back(entity);
            m_Worker->scheduleChunk(coord, entity);
        }
    }
}

void EC_VoxelChunkSystem::loadVolumeScript(EC_Game& game) {
    // Run the generation script once, synchronously, before scheduling chunk jobs, then hand the tree
    // to the worker. The tree is stateless once built (see EC_VolumeNode), so the worker thread can
    // evaluate it safely.
    if (!game.runLuaScriptOnce(m_GenerationScriptPath)) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_VoxelChunkSystem: failed to run " + m_GenerationScriptPath + " - chunks will generate as empty space",
            LOGGING::LogLevel::SEVERE);
    }
    m_Worker->setVolumeRoot(game.getVolumeRoot());
}

void EC_VoxelChunkSystem::regenerate(EC_Game& game) {
    if (!m_Worker) return;

    loadVolumeScript(game);

    auto& manager = EC_DOD_EntityManager::getInstance();
    for (EntityID entity : m_ChunkEntities) {
        if (!manager.isAlive(entity)) continue;
        if (!manager.hasComponent<EC_DOD_VoxelChunk>(entity)) continue;

        auto& chunk = manager.getComponent<EC_DOD_VoxelChunk>(entity);
        chunk.state = EC_DOD_VoxelChunk::State::Generating;
        m_Worker->scheduleChunk(chunk.chunkCoord, entity);
    }

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "EC_VoxelChunkSystem: regenerating " + std::to_string(m_ChunkEntities.size()) + " chunks from " + m_GenerationScriptPath,
        LOGGING::LogLevel::INFORMATION);
}

void EC_VoxelChunkSystem::update(const float& deltaTimeS, EC_Game& game) {
    if (!m_Worker) return;

    if (m_RegenerateRequested.exchange(false)) {
        regenerate(game);
    }

    auto& manager = EC_DOD_EntityManager::getInstance();

    bool voxelSceneActive = game.isSceneActive("voxelchunkdemo");
    for (EntityID chunkEntity : m_ChunkEntities) {
        if (!manager.isAlive(chunkEntity)) continue;
        if (!manager.hasComponent<EC_DOD_EntityInfo>(chunkEntity)) continue;
        manager.getComponent<EC_DOD_EntityInfo>(chunkEntity).sceneState =
            voxelSceneActive ? EC_SceneLifecycleState::Active : EC_SceneLifecycleState::Inactive;
    }

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

        // Renderable so EC_BroadPhase's spatial index (and frustum culling) can find the chunk;
        // constructEntity() does this for XML entities but chunks bypass it. Type::Mesh so ray/cone queries
        // hit the real surface; center/extents still give the broad-phase box.
        EC_DOD_Collider collider;
        collider.type = EC_DOD_Collider::Type::Mesh;
        float half = static_cast<float>(kChunkWorldSize) * 0.5f;
        collider.extents = glm::vec3(half);
        collider.center = glm::vec3(half);
        collider.collisionLayer |= CollisionLayers::Renderable;
        manager.addComponent(entity, collider);

        size_t vertexCount = meshData.positions.size(); // meshData is moved-from below

        // Retained for ray/cone testing (EC_RayIntersection::rayMesh,
        // EC_BroadPhase::handleConeCheck) - ObjModel keeps no public accessor to the
        // vertex data it just uploaded, so this is the only copy that survives past this
        // function.
        EC_DOD_MeshCollisionData meshCollision;
        meshCollision.positions = std::make_shared<std::vector<glm::vec3>>(std::move(meshData.positions));
        meshCollision.normals = std::make_shared<std::vector<glm::vec3>>(std::move(meshData.normals));
        meshCollision.indices = std::make_shared<std::vector<uint32_t>>(std::move(meshData.indices));
        manager.addComponent(entity, meshCollision);

        manager.getComponent<EC_DOD_VoxelChunk>(entity).state = EC_DOD_VoxelChunk::State::Ready;

        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_VoxelChunkSystem: chunk ready (" + std::to_string(vertexCount) + " vertices)",
            LOGGING::LogLevel::INFORMATION);
    }
}
