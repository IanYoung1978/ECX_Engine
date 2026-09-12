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
        void shutdown();
        void pauseGame();
        void resumeGame();
        int getKeyState(const std::string& key);

        void setParent(unsigned int childID, unsigned int parentID);
        void clearParent(unsigned int childID);

        void setExposure(float exposure);
        // Issue #96 - scales the derived (from active scene lights, not authored) ambient
        // colour. See GL_Deferred_Renderer.h's m_DerivedAmbientColour/m_AmbientScale.
        void setAmbientScale(float scale);
        void toggleDebug();
        // Issue #108. Requests a window/render resolution change - see Game.h's
        // setResolution() for why this goes through a command rather than a direct call.
        void setResolution(int width, int height);
        void toggleFullscreen();
        void maximizeWindow();
        void minimizeWindow();

        void loadScene(const std::string& alias);
        void unloadScene(const std::string& alias);
        void activateScene(const std::string& alias);

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
        void setMouseCaptured(bool captured);
        void log(const std::string& message);

        // Issue #30. Returns all entities the ray intersects (not just the nearest) unless
        // firstHitOnly is set. Caches the result for the paginated getters below - avoids
        // marshaling a vector-of-struct across the Lua boundary, matching the
        // getRecentLogCount/getRecentLog pattern already used for the debug overlay.
        int rayQuery(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance, bool firstHitOnly = false);
        EntityAPI getRayHitEntity(int index);
        glm::vec3 getRayHitPosition(int index);
        glm::vec3 getRayHitNormal(int index);
        float getRayHitDistance(int index);

        // Issue #29. Entities whose shape overlaps the cone, restricted to castsShadow ==
        // true geometry by default. checkOcclusion opts into additionally requiring
        // unobstructed line-of-sight to the apex (a candidate stacked behind a closer one
        // is excluded) - independent of containment, not fused into it. Same caching
        // pattern as rayQuery above.
        int coneQuery(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance, bool castsShadowOnly = true, bool checkOcclusion = false);
        EntityAPI getConeHitEntity(int index);
        glm::vec3 getConeHitPosition(int index);
        float getConeHitDistance(int index);

        // Visualizes the last ray/cone query (Issues #30/#29) - a debug draw only, no
        // effect on collision/query behaviour. Persists until replaced by another call.
        void showDebugRay(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance);
        void showDebugCone(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance);

        // Re-runs TerrainGeneration.lua and re-schedules every voxel chunk against the new
        // shape - see EC_VoxelChunkSystem::requestRegenerate(). Safe to call from any
        // script context; only takes effect on the next chunk-system update tick.
        void regenerateTerrain();

        // Real capsule-vs-scene-geometry overlap query (see EC_Game::queryCapsule) -
        // includes real Mesh/terrain collision via EC_CollisionChecks::CapsuleVsMesh, not
        // a raycast stand-in. Static overlap test, not a sweep: getCapsuleHitDistance
        // returns penetration depth, and getCapsuleHitNormal points away from the capsule
        // toward whatever it's touching. Same caching pattern as rayQuery/coneQuery above.
        // excludeEntityId skips one entity (its own ID, typically) so a capsule querying
        // its own surroundings doesn't find itself - pass 0 (INVALID_ENTITY) for no
        // exclusion.
        int capsuleQuery(float ax, float ay, float az, float bx, float by, float bz, float radius,
            bool firstHitOnly = false, unsigned int excludeEntityId = 0);
        EntityAPI getCapsuleHitEntity(int index);
        glm::vec3 getCapsuleHitPosition(int index);
        glm::vec3 getCapsuleHitNormal(int index);
        float getCapsuleHitDistance(int index);

        // Issue #112. See EC_AudioSystem.h for what each call does - category is any
        // author-chosen name ("sfx", "music", ...) plus the reserved "master" for the
        // overall engine volume.
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
