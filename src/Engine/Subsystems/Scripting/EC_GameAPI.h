#pragma once
#include "Entity/EC_DOD_Types.h"
#include "Spatial/RayQueryHit.h"
#include <string>
#include <vector>
#include <glm/glm.hpp>

class EC_Game;
class ECXMessenger;
namespace ScriptAPI { struct EntityAPI; }

namespace ScriptAPI
{
    struct GameAPI
    {
        EC_Game* game;

        ECXMessenger* messenger;
        GameAPI(EC_Game* g, ECXMessenger& m) : game(g), messenger(&m) {}

        EntityAPI getEntityByName(const std::string& name);
        unsigned int getEntityIDByUID(unsigned int uid);
        // Wraps a raw entity ID (e.g. from EventAPI::getCollisionEntityA/B) as an EntityAPI,
        // so a collision handler can call .getName()/.getVelocity()/etc. on whatever it hit
        // without a separate name/UID lookup. Invalid/dead IDs behave like any other
        // EntityAPI on a dead entity (calls are no-ops / return defaults).
        EntityAPI getEntityByID(unsigned int id);
        void shutdown();
        void pauseGame();
        void resumeGame();
        int getKeyState(const std::string& key);

        void setParent(unsigned int childID, unsigned int parentID);
        void clearParent(unsigned int childID);

        void setExposure(float exposure);
        // Scales the ambient colour derived from the active scene lights (see GL_Deferred_Renderer.h).
        void setAmbientScale(float scale);
        void toggleDebug();
        // Requests a window/render resolution change through a command (see Game.h's setResolution()).
        void setResolution(int width, int height);
        void toggleFullscreen();
        void maximizeWindow();
        void minimizeWindow();

        void loadScene(const std::string& alias);
        void unloadScene(const std::string& alias);
        void activateScene(const std::string& alias);
        // Returns an invalid EntityAPI (getID() == 0) if the alias is unknown or the prefab fails to load.
        // destroyEntity takes a raw ID, like setParent/clearParent.
        EntityAPI spawnEntity(const std::string& alias, float x, float y, float z);
        void destroyEntity(unsigned int entityId);
        // Scene state queries, so scripts don't have to track it themselves.
        bool isSceneActive(const std::string& alias);

        void setUIText(unsigned int entityID, const std::string& text);
        void setUITextColour(unsigned int entityID, float r, float g, float b, float a);
        void setUIPanelColour(unsigned int entityID, float r, float g, float b, float a);
        void setUIVisible(unsigned int entityID, bool visible);
        void setUIPosition(unsigned int entityID, float x, float y);
        void setUISize(unsigned int entityID, float w, float h);
        void setUILayer(unsigned int entityID, int layer);
        unsigned int createUIElement(float x, float y, float w, float h, int layer);

        float getFPS();
        float getMSPF();
        int getRecentLogCount();
        std::string getRecentLog(int index);

        // Real updates per second for each background task (Physics, Scripting, Audio), to see which
        // fixed-timestep thread is falling behind (see EC_SystemTask::getTickRate()). Paginated getters like
        // getRecentLogCount/getRecentLog.
        int getThreadCount();
        std::string getThreadName(int index);
        float getThreadFPS(int index);
        void setMouseCaptured(bool captured);
        void log(const std::string& message);

        // Polling-style input for update() loops, unlike EventAPI's handler-only getters. button matches
        // EventAPI::getMouseButton() (0=LMB, 1=RMB, 2=Middle, 3=MB4, 4=MB5).
        glm::vec2 getMousePosition();
        bool isMouseButtonPressed(int button);

        // Returns all entities the ray intersects unless firstHitOnly is set. The result is cached for the
        // paginated getters below rather than marshaled across the Lua boundary as a vector of structs.
        int rayQuery(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance, bool firstHitOnly = false);
        EntityAPI getRayHitEntity(int index);
        glm::vec3 getRayHitPosition(int index);
        glm::vec3 getRayHitNormal(int index);
        float getRayHitDistance(int index);

        // Entities whose shape overlaps the cone, castsShadow geometry only by default. checkOcclusion
        // additionally requires an unobstructed line of sight to the apex. Cached like rayQuery.
        int coneQuery(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance, bool castsShadowOnly = true, bool checkOcclusion = false);
        EntityAPI getConeHitEntity(int index);
        glm::vec3 getConeHitPosition(int index);
        float getConeHitDistance(int index);

        // Debug draw of the last ray/cone query; no effect on collision or queries. Persists until replaced.
        void showDebugRay(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance);
        void showDebugCone(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance);

        // Re-runs the terrain generation script and reschedules every chunk (see
        // EC_VoxelChunkSystem::requestRegenerate()); takes effect on the next chunk-system update.
        void regenerateTerrain();

        // Capsule-vs-scene overlap (see EC_Game::queryCapsule), including Mesh/terrain. A static overlap
        // test, not a sweep: getCapsuleHitDistance is penetration depth and getCapsuleHitNormal points away
        // from the capsule. Cached like rayQuery/coneQuery. excludeEntityId skips one entity (typically the
        // caller's own ID); 0 excludes none.
        int capsuleQuery(float ax, float ay, float az, float bx, float by, float bz, float radius,
            bool firstHitOnly = false, unsigned int excludeEntityId = 0);
        EntityAPI getCapsuleHitEntity(int index);
        glm::vec3 getCapsuleHitPosition(int index);
        glm::vec3 getCapsuleHitNormal(int index);
        float getCapsuleHitDistance(int index);

        // See EC_AudioSystem.h. category is any author-chosen name ("sfx", "music", ...) plus the
        // reserved "master".
        void playSound(const std::string& path, float volume, const std::string& category);
        void playMusic(const std::string& path, float volume, bool loop);
        void stopMusic();
        void setCategoryVolume(const std::string& category, float volume);

    private:
        std::vector<RayQueryHit> m_LastRayHits;
        std::vector<RayQueryHit> m_LastConeHits;
        std::vector<RayQueryHit> m_LastCapsuleHits;
        void updateDepth(EntityID entity, uint32_t depth);
    };
}
