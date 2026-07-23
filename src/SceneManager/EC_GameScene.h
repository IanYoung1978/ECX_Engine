#pragma once
#include <vector>
#include <mutex>
#include "Entity/EC_DOD_Types.h"
#include <string>
#include <glm/glm.hpp>

class EC_GameScene
{
public:
    EC_GameScene() = default;
    EC_GameScene(const EC_GameScene&) = delete;
    EC_GameScene& operator=(const EC_GameScene&) = delete;
    EC_GameScene(EC_GameScene&& other) noexcept;
    EC_GameScene& operator=(EC_GameScene&& other) noexcept;

    void activate();
    void deactivate();
    void unload();

    void addEntity(EntityID id);
    void addCamera(EntityID id);
    void addLight(EntityID id);

    std::vector<EntityID> getEntities() const;
    std::vector<EntityID> getCameras()  const;
    std::vector<EntityID> getLights()   const;

    const std::string& getAlias()    const { return m_Alias; }
    const std::string& getFilename() const { return m_Filename; }
    bool isPrecached()           const { return m_Precache; }
    bool isUnloadOnDeactivate()  const { return m_UnloadOnDeactivate; }
    bool isLoaded()              const { return m_Loaded; }

    void setAlias(const std::string& alias) { m_Alias = alias; }
    void setFilename(const std::string& filename) { m_Filename = filename; }
    void setPrecache(bool v) { m_Precache = v; }
    void setUnloadOnDeactivate(bool v) { m_UnloadOnDeactivate = v; }
    void setLoaded(bool v) { m_Loaded = v; }

    bool hasStreamTrigger()      const { return m_HasStreamTrigger; }
    const glm::vec3& getTriggerPosition() const { return m_TriggerPosition; }
    float getLoadRadius()        const { return m_LoadRadius; }
    float getActivateRadius()    const { return m_ActivateRadius; }
    void setStreamTrigger(const glm::vec3& position, float loadRadius, float activateRadius)
    {
        m_TriggerPosition = position;
        m_LoadRadius = loadRadius;
        m_ActivateRadius = activateRadius;
        m_HasStreamTrigger = true;
    }

private:
    std::string m_Alias;
    std::string m_Filename;
    bool m_Precache = false;
    bool m_UnloadOnDeactivate = true;
    bool m_Loaded = false;
    std::vector<EntityID> m_Entities;
    std::vector<EntityID> m_Cameras;
    std::vector<EntityID> m_Lights;
    mutable std::mutex m_Mutex;

    bool m_HasStreamTrigger = false;
    glm::vec3 m_TriggerPosition{};
    float m_LoadRadius = 0.f;
    float m_ActivateRadius = 0.f;
};