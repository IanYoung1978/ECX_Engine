#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <shared_mutex>
#include <cstdint>
#include "EC_DOD_Types.h"

class EC_IComponentArray {
public:
    virtual ~EC_IComponentArray() = default;
    virtual void removeEntity(EntityID entity) = 0;
    virtual bool hasEntity(EntityID entity) const = 0;
    virtual size_t size() const = 0;
};

template<typename T>
class EC_ComponentArray : public EC_IComponentArray {
public:
    EC_ComponentArray();

    void insert(EntityID entity, const T& component);
    void remove(EntityID entity);
    bool has(EntityID entity) const;

    T& get(EntityID entity);
    const T& get(EntityID entity) const;

    std::vector<T>& getData();
    const std::vector<T>& getData() const;

    EntityID getEntity(size_t index) const;

    void removeEntity(EntityID entity) override;
    bool hasEntity(EntityID entity) const override;
    size_t size() const override;

    std::shared_mutex& getMutex();

private:
    std::vector<T> m_Components;
    std::vector<EntityID> m_Entities;
    std::unordered_map<EntityID, size_t> m_EntityToIndex;
    mutable std::shared_mutex m_Mutex;
};

class EC_DOD_EntityManager {
public:
    EC_DOD_EntityManager();
    EC_DOD_EntityManager(const EC_DOD_EntityManager&) = delete;
    EC_DOD_EntityManager& operator=(const EC_DOD_EntityManager&) = delete;

    static EC_DOD_EntityManager& getInstance();

    ~EC_DOD_EntityManager();

    EntityID createEntity();
    void destroyEntity(EntityID entity);
    bool isAlive(EntityID entity) const;

    template<typename T>
    void addComponent(EntityID entity, const T& component);

    template<typename T>
    void removeComponent(EntityID entity);

    template<typename T>
    bool hasComponent(EntityID entity) const;

    template<typename T>
    T& getComponent(EntityID entity);

    template<typename T>
    const T& getComponent(EntityID entity) const;

    template<typename T>
    EC_ComponentArray<T>* getComponentArray();

    template<typename T>
    const EC_ComponentArray<T>* getComponentArray() const;

    std::vector<EntityID> getEntitiesWithComponent(std::type_index type) const;
    std::vector<EntityID> getEntitiesWithComponents(const std::vector<std::type_index>& types) const;

    // Same as getEntitiesWithComponents, further filtered to entities whose
    // EC_DOD_EntityInfo has both `active` (gameplay-level) and `sceneActive`
    // (scene-membership) set - the "should this entity actually participate right now"
    // check every subsystem needs, consolidated here instead of each one re-implementing
    // its own isAlive+hasComponent<EntityInfo>+.active loop (see EC_DOD_EntityInfo's
    // comment for why the two flags are separate). An entity with no EC_DOD_EntityInfo at
    // all is treated as active (permissive default - every entity constructed via the
    // normal factory path has one, so this only matters for hand-built test entities).
    std::vector<EntityID> getActiveEntitiesWithComponents(const std::vector<std::type_index>& types) const;

    void clear();

    size_t getEntityCount() const;
    size_t getAliveEntityCount() const;

private:
    void reclaimTombstones();
	static EC_DOD_EntityManager* s_Instance;
    EntityID m_NextEntityID;
    std::vector<EntityID> m_FreeList;
    std::vector<EntityID> m_Tombstones;
    std::vector<EntityID> m_AliveEntities;

    std::unordered_map<std::type_index, std::shared_ptr<EC_IComponentArray>> m_ComponentArrays;
    mutable std::shared_mutex m_ComponentArraysMutex;

    mutable std::shared_mutex m_EntityMutex;

    static constexpr size_t TOMBSTONE_FRAMES = 2;
    size_t m_FrameCount;

};

template<typename T>
EC_ComponentArray<T>::EC_ComponentArray() {
    m_Components.reserve(256);
    m_Entities.reserve(256);
}

template<typename T>
void EC_ComponentArray<T>::insert(EntityID entity, const T& component) {
    std::unique_lock lock(m_Mutex);

    auto it = m_EntityToIndex.find(entity);
    if (it != m_EntityToIndex.end()) {
        m_Components[it->second] = std::move(component);
        return;
    }

    size_t newIndex = m_Components.size();
    m_EntityToIndex[entity] = newIndex;
    m_Components.push_back(component);
    m_Entities.push_back(entity);
}

template<typename T>
void EC_ComponentArray<T>::remove(EntityID entity) {
    std::unique_lock lock(m_Mutex);
    removeEntity(entity);
}

template<typename T>
bool EC_ComponentArray<T>::has(EntityID entity) const {
    std::shared_lock lock(m_Mutex);
    return m_EntityToIndex.find(entity) != m_EntityToIndex.end();
}

template<typename T>
T& EC_ComponentArray<T>::get(EntityID entity) {
    std::shared_lock lock(m_Mutex);
    auto it = m_EntityToIndex.find(entity);
    if (it == m_EntityToIndex.end()) {
        throw std::runtime_error("Entity does not have component");
    }
    return m_Components[it->second];
}

template<typename T>
const T& EC_ComponentArray<T>::get(EntityID entity) const {
    std::shared_lock lock(m_Mutex);
    auto it = m_EntityToIndex.find(entity);
    if (it == m_EntityToIndex.end()) {
        throw std::runtime_error("Entity does not have component");
    }
    return m_Components[it->second];
}

template<typename T>
std::vector<T>& EC_ComponentArray<T>::getData() {
    return m_Components;
}

template<typename T>
const std::vector<T>& EC_ComponentArray<T>::getData() const {
    return m_Components;
}

template<typename T>
EntityID EC_ComponentArray<T>::getEntity(size_t index) const {
    std::shared_lock lock(m_Mutex);
    return m_Entities[index];
}

template<typename T>
void EC_ComponentArray<T>::removeEntity(EntityID entity) {
    auto it = m_EntityToIndex.find(entity);
    if (it == m_EntityToIndex.end()) {
        return;
    }

    size_t indexToRemove = it->second;
    size_t lastIndex = m_Components.size() - 1;

    if (indexToRemove != lastIndex) {
        m_Components[indexToRemove] = std::move(m_Components[lastIndex]);
        m_Entities[indexToRemove] = m_Entities[lastIndex];

        EntityID lastEntity = m_Entities[lastIndex];
        m_EntityToIndex[lastEntity] = indexToRemove;
    }

    m_Components.pop_back();
    m_Entities.pop_back();
    m_EntityToIndex.erase(entity);
}

template<typename T>
bool EC_ComponentArray<T>::hasEntity(EntityID entity) const {
    return has(entity);
}

template<typename T>
size_t EC_ComponentArray<T>::size() const {
    std::shared_lock lock(m_Mutex);
    return m_Components.size();
}

template<typename T>
std::shared_mutex& EC_ComponentArray<T>::getMutex() {
    return m_Mutex;
}

template<typename T>
void EC_DOD_EntityManager::addComponent(EntityID entity, const T& component) {
    auto array = getComponentArray<T>();
    if (array) {
        array->insert(entity, component);
    }
}

template<typename T>
void EC_DOD_EntityManager::removeComponent(EntityID entity) {
    std::type_index typeIndex(typeid(T));
    auto it = m_ComponentArrays.find(typeIndex);
    if (it != m_ComponentArrays.end()) {
        it->second->removeEntity(entity);
    }
}

template<typename T>
bool EC_DOD_EntityManager::hasComponent(EntityID entity) const {
    std::type_index typeIndex(typeid(T));
    auto it = m_ComponentArrays.find(typeIndex);
    if (it == m_ComponentArrays.end()) {
        return false;
    }
    return it->second->hasEntity(entity);
}

template<typename T>
T& EC_DOD_EntityManager::getComponent(EntityID entity) {
    auto array = getComponentArray<T>();
    if (!array) {
        throw std::runtime_error("Component type not registered");
    }
    return array->get(entity);
}

template<typename T>
const T& EC_DOD_EntityManager::getComponent(EntityID entity) const {
    auto array = getComponentArray<T>();
    if (!array) {
        throw std::runtime_error("Component type not registered");
    }
    return array->get(entity);
}

template<typename T>
EC_ComponentArray<T>* EC_DOD_EntityManager::getComponentArray() {
    std::type_index typeIndex(typeid(T));

    {
        std::shared_lock readLock(m_ComponentArraysMutex);
        auto it = m_ComponentArrays.find(typeIndex);
        if (it != m_ComponentArrays.end())
            return static_cast<EC_ComponentArray<T>*>(it->second.get());
    }

    std::unique_lock writeLock(m_ComponentArraysMutex);
    auto it = m_ComponentArrays.find(typeIndex);
    if (it != m_ComponentArrays.end())
        return static_cast<EC_ComponentArray<T>*>(it->second.get());

    auto newArray = std::make_shared<EC_ComponentArray<T>>();
    m_ComponentArrays[typeIndex] = newArray;
    return newArray.get();
}

template<typename T>
const EC_ComponentArray<T>* EC_DOD_EntityManager::getComponentArray() const {
    std::type_index typeIndex(typeid(T));
    std::shared_lock readLock(m_ComponentArraysMutex);
    auto it = m_ComponentArrays.find(typeIndex);
    if (it == m_ComponentArrays.end()) {
        return nullptr;
    }
    return static_cast<const EC_ComponentArray<T>*>(it->second.get());
}
