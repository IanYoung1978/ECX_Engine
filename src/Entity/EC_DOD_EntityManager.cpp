#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include <algorithm>


EC_DOD_EntityManager* EC_DOD_EntityManager::s_Instance = nullptr;

EC_DOD_EntityManager& EC_DOD_EntityManager::getInstance() {
    if (s_Instance == nullptr) {
        s_Instance = new EC_DOD_EntityManager();
    }
    return *s_Instance;
}

EC_DOD_EntityManager::EC_DOD_EntityManager()
    : m_NextEntityID(1)
    , m_FrameCount(0)
{
    m_AliveEntities.reserve(256);
}

EC_DOD_EntityManager::~EC_DOD_EntityManager() {
    clear();
	delete s_Instance;
}

EntityID EC_DOD_EntityManager::createEntity() {
    std::unique_lock lock(m_EntityMutex);

    EntityID id;

    if (!m_FreeList.empty()) {
        id = m_FreeList.back();
        m_FreeList.pop_back();
    }
    else {
        id = m_NextEntityID++;
    }

    m_AliveEntities.push_back(id);
    return id;
}

void EC_DOD_EntityManager::destroyEntity(EntityID entity) {
    if (entity == INVALID_ENTITY) {
        return;
    }

    {
        std::unique_lock lock(m_EntityMutex);

        auto it = std::find(m_AliveEntities.begin(), m_AliveEntities.end(), entity);
        if (it != m_AliveEntities.end()) {
            m_AliveEntities.erase(it);
        }

        m_Tombstones.push_back(entity);
    }

    for (auto& [type, array] : m_ComponentArrays) {
        array->removeEntity(entity);
    }

    if (m_FrameCount % TOMBSTONE_FRAMES == 0) {
        reclaimTombstones();
    }
}

bool EC_DOD_EntityManager::isAlive(EntityID entity) const {
    if (entity == INVALID_ENTITY) {
        return false;
    }

    std::shared_lock lock(m_EntityMutex);
    return std::find(m_AliveEntities.begin(), m_AliveEntities.end(), entity) != m_AliveEntities.end();
}

std::vector<EntityID> EC_DOD_EntityManager::getEntitiesWithComponent(std::type_index type) const {
    auto it = m_ComponentArrays.find(type);
    if (it == m_ComponentArrays.end()) {
        return {};
    }

    std::vector<EntityID> result;
    result.reserve(it->second->size());

    for (size_t i = 0; i < it->second->size(); i++) {
        EntityID entity = static_cast<const EC_ComponentArray<int>*>(it->second.get())->getEntity(i);
        if (isAlive(entity)) {
            result.push_back(entity);
        }
    }

    return result;
}

std::vector<EntityID> EC_DOD_EntityManager::getEntitiesWithComponents(const std::vector<std::type_index>& types) const {
    if (types.empty()) {
        return {};
    }

    std::vector<EntityID> result;

    auto smallestArrayIt = m_ComponentArrays.end();
    size_t smallestSize = SIZE_MAX;

    for (const auto& type : types) {
        auto it = m_ComponentArrays.find(type);
        if (it == m_ComponentArrays.end()) {
            return {};
        }

        size_t arraySize = it->second->size();
        if (arraySize < smallestSize) {
            smallestSize = arraySize;
            smallestArrayIt = it;
        }
    }

    if (smallestArrayIt == m_ComponentArrays.end()) {
        return {};
    }

    result.reserve(smallestSize);

    for (size_t i = 0; i < smallestArrayIt->second->size(); i++) {
        EntityID entity = static_cast<const EC_ComponentArray<int>*>(smallestArrayIt->second.get())->getEntity(i);

        if (!isAlive(entity)) {
            continue;
        }

        bool hasAll = true;
        for (const auto& type : types) {
            auto it = m_ComponentArrays.find(type);
            if (it == m_ComponentArrays.end() || !it->second->hasEntity(entity)) {
                hasAll = false;
                break;
            }
        }

        if (hasAll) {
            result.push_back(entity);
        }
    }

    return result;
}

std::vector<EntityID> EC_DOD_EntityManager::getActiveEntitiesWithComponents(const std::vector<std::type_index>& types) const {
    std::vector<EntityID> candidates = getEntitiesWithComponents(types);

    std::vector<EntityID> result;
    result.reserve(candidates.size());

    for (EntityID entity : candidates) {
        if (hasComponent<EC_DOD_EntityInfo>(entity)) {
            const auto& info = getComponent<EC_DOD_EntityInfo>(entity);
            if (!info.active || !info.sceneActive) continue;
        }
        result.push_back(entity);
    }

    return result;
}

void EC_DOD_EntityManager::clear() {
    std::unique_lock lock(m_EntityMutex);

    m_ComponentArrays.clear();
    m_AliveEntities.clear();
    m_FreeList.clear();
    m_Tombstones.clear();
    m_NextEntityID = 1;
    m_FrameCount = 0;
}

size_t EC_DOD_EntityManager::getEntityCount() const {
    std::shared_lock lock(m_EntityMutex);
    return m_NextEntityID - 1;
}

size_t EC_DOD_EntityManager::getAliveEntityCount() const {
    std::shared_lock lock(m_EntityMutex);
    return m_AliveEntities.size();
}

void EC_DOD_EntityManager::reclaimTombstones() {
    std::unique_lock lock(m_EntityMutex);

    m_FreeList.insert(m_FreeList.end(), m_Tombstones.begin(), m_Tombstones.end());
    m_Tombstones.clear();

    m_FrameCount++;
}