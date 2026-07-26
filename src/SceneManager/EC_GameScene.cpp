#include "EC_GameScene.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"

// std::mutex is neither copyable nor movable, so the move ops below construct a fresh mutex on
// this instance and move everything else. Only used by std::vector<EC_GameScene>'s internal
// machinery (e.g. reserve()), which requires move-constructibility to compile even when no
// reallocation actually happens at runtime.
EC_GameScene::EC_GameScene(EC_GameScene&& other) noexcept
{
    std::lock_guard<std::mutex> lock(other.m_Mutex);
    m_Alias = std::move(other.m_Alias);
    m_Filename = std::move(other.m_Filename);
    m_Precache = other.m_Precache;
    m_UnloadOnDeactivate = other.m_UnloadOnDeactivate;
    m_Loaded = other.m_Loaded;
    m_Entities = std::move(other.m_Entities);
    m_Cameras = std::move(other.m_Cameras);
    m_Lights = std::move(other.m_Lights);
}

EC_GameScene& EC_GameScene::operator=(EC_GameScene&& other) noexcept
{
    if (this == &other)
        return *this;

    std::scoped_lock lock(m_Mutex, other.m_Mutex);
    m_Alias = std::move(other.m_Alias);
    m_Filename = std::move(other.m_Filename);
    m_Precache = other.m_Precache;
    m_UnloadOnDeactivate = other.m_UnloadOnDeactivate;
    m_Loaded = other.m_Loaded;
    m_Entities = std::move(other.m_Entities);
    m_Cameras = std::move(other.m_Cameras);
    m_Lights = std::move(other.m_Lights);
    return *this;
}

void EC_GameScene::activate()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& manager = EC_DOD_EntityManager::getInstance();
    for (EntityID entity : m_Entities)
    {
        if (manager.isAlive(entity) && manager.hasComponent<EC_DOD_EntityInfo>(entity))
            manager.getComponent<EC_DOD_EntityInfo>(entity).active = true;
    }
}

void EC_GameScene::deactivate()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto& manager = EC_DOD_EntityManager::getInstance();
    for (EntityID entity : m_Entities)
    {
        if (manager.isAlive(entity) && manager.hasComponent<EC_DOD_EntityInfo>(entity))
            manager.getComponent<EC_DOD_EntityInfo>(entity).active = false;
    }
}

void EC_GameScene::unload()
{
    std::vector<EntityID> entities;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        entities = std::move(m_Entities);
        m_Entities.clear();
        m_Cameras.clear();
        m_Lights.clear();
    }

    auto& manager = EC_DOD_EntityManager::getInstance();
    for (EntityID entity : entities)
    {
        if (manager.isAlive(entity))
            manager.destroyEntity(entity);
    }
    m_Loaded = false;
}

void EC_GameScene::addEntity(EntityID id)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Entities.push_back(id);
}

void EC_GameScene::addCamera(EntityID id)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Cameras.push_back(id);
}

void EC_GameScene::addLight(EntityID id)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Lights.push_back(id);
}

std::vector<EntityID> EC_GameScene::getEntities() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Entities;
}

std::vector<EntityID> EC_GameScene::getCameras() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Cameras;
}

std::vector<EntityID> EC_GameScene::getLights() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Lights;
}
