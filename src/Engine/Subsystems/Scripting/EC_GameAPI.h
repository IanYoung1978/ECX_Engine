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
        void toggleDebug();

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

    private:
        std::vector<RayQueryHit> m_LastRayHits;
        std::vector<RayQueryHit> m_LastConeHits;
        void updateDepth(EntityID entity, uint32_t depth);
    };
}
