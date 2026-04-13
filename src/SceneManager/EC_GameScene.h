#pragma once
#include <vector>
#include "Entity/EC_DOD_Types.h"
#include <string>

class EC_GameScene
{
public:
    EC_GameScene() = default;

    void activate();
    void deactivate();

    void addEntity(EntityID id) { m_Entities.push_back(id); }
    void addCamera(EntityID id) { m_Cameras.push_back(id); }
    void addLight(EntityID id) { m_Lights.push_back(id); }

    const std::vector<EntityID>& getEntities() const { return m_Entities; }
    const std::vector<EntityID>& getCameras()  const { return m_Cameras; }
    const std::vector<EntityID>& getLights()   const { return m_Lights; }

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

private:
    std::string m_Alias;
    std::string m_Filename;
    bool m_Precache = false;
    bool m_UnloadOnDeactivate = true;
    bool m_Loaded = false;
    std::vector<EntityID> m_Entities;
    std::vector<EntityID> m_Cameras;
    std::vector<EntityID> m_Lights;
};