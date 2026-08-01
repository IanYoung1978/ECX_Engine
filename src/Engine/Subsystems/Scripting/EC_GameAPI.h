#pragma once
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Entity/EC_DOD_Types.h"
#include "Engine/Subsystems/Scripting/EC_EntityAPI.h"
#include "UI/EC_UI_Components.h"
#include "Logging/ECX_Logging.h"
#include "Spatial/RayQueryHit.h"
#include "Graphics/DebugVisualization.h"
#include "Game.h"
#include <string>
#include <algorithm>
#include <vector>
#include "Messaging/ECXMessenger.h"
class EC_Game;

namespace ScriptAPI
{
    struct GameAPI
    {
        EC_Game* game;

        ECXMessenger* messenger;
        GameAPI(EC_Game* g, ECXMessenger& m) : game(g), messenger(&m) {}

        EntityAPI getEntityByName(const std::string& name) {
            if (!game) return EntityAPI(INVALID_ENTITY);
            return EntityAPI(game->getEntityByName(name));
        }

        unsigned int getEntityIDByUID(unsigned int uid) {
            if (!game) return INVALID_ENTITY;
            return game->getEntityByUID(static_cast<uint32_t>(uid));
        }
        void shutdown() {
            if (game) game->shutDown();
        }

        void pauseGame() {
            if (game) game->pauseGame();
        }

        void resumeGame() {
            if (game) game->resumeGame();
        }

        int getKeyState(const std::string& key) {
            if (!game) return 0;
            SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());
            KeyState state = game->getKeyState(scancode);
            return static_cast<int>(state);
        }

        void setParent(unsigned int childID, unsigned int parentID) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID child = static_cast<EntityID>(childID);
            EntityID parent = static_cast<EntityID>(parentID);

            if (!mgr.isAlive(child)) return;
            if (!mgr.isAlive(parent)) return;

            if (!mgr.hasComponent<EC_DOD_Hierarchy>(child))
                mgr.addComponent(child, EC_DOD_Hierarchy{});
            if (!mgr.hasComponent<EC_DOD_Hierarchy>(parent))
                mgr.addComponent(parent, EC_DOD_Hierarchy{});

            auto& childHierarchy = mgr.getComponent<EC_DOD_Hierarchy>(child);
            EntityID oldParent = childHierarchy.parent;
            if (oldParent == parent) return;

            // Remove from old parent
            if (oldParent != INVALID_ENTITY && mgr.hasComponent<EC_DOD_Hierarchy>(oldParent)) {
                auto& oldParentHierarchy = mgr.getComponent<EC_DOD_Hierarchy>(oldParent);
                auto& children = oldParentHierarchy.children;
                children.erase(std::remove(children.begin(), children.end(), child), children.end());
            }

            // Set new parent
            childHierarchy.parent = parent;
            mgr.getComponent<EC_DOD_Hierarchy>(parent).children.push_back(child);

            // Update depths
            updateDepth(child, mgr.getComponent<EC_DOD_Hierarchy>(parent).depth + 1);

            // Mark transform dirty
            if (mgr.hasComponent<EC_DOD_Transform>(child))
                mgr.getComponent<EC_DOD_Transform>(child).dirty = true;
        }

        void clearParent(unsigned int childID) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID child = static_cast<EntityID>(childID);

            if (!mgr.isAlive(child)) return;
            if (!mgr.hasComponent<EC_DOD_Hierarchy>(child)) return;

            auto& hierarchy = mgr.getComponent<EC_DOD_Hierarchy>(child);
            EntityID oldParent = hierarchy.parent;
            if (oldParent == INVALID_ENTITY) return;

            // Remove from old parent's children list
            if (mgr.hasComponent<EC_DOD_Hierarchy>(oldParent)) {
                auto& children = mgr.getComponent<EC_DOD_Hierarchy>(oldParent).children;
                children.erase(std::remove(children.begin(), children.end(), child), children.end());
            }

            // Clear parent and reset depth
            hierarchy.parent = INVALID_ENTITY;
            updateDepth(child, 0);

            // Mark transform dirty
            if (mgr.hasComponent<EC_DOD_Transform>(child))
                mgr.getComponent<EC_DOD_Transform>(child).dirty = true;
        }
        void setExposure(float exposure) {
            if (!messenger) return;
            ECXCommand cmd;
            cmd.type = ECXCommandType::GraphicsChangeHDRExposure;
            cmd.args[0] = exposure;
            messenger->publish(cmd);
        }
        void toggleDebug() {
            if (!messenger) return;
            ECXCommand cmd;
            cmd.type = ECXCommandType::GraphicsToggleDebug;
            messenger->publish(cmd);
		}
        void loadScene(const std::string& alias) {
            if (game) game->loadScene(alias);
        }

        void unloadScene(const std::string& alias) {
            if (game) game->unloadScene(alias);
        }

        void activateScene(const std::string& alias) {
            if (game) game->activateScene(alias);
        }

        void setUIText(unsigned int entityID, const std::string& text) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = static_cast<EntityID>(entityID);
            if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Text>(entity)) return;
            mgr.getComponent<EC_UI_Text>(entity).text = text;
        }

        void setUITextColour(unsigned int entityID, float r, float g, float b, float a) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = static_cast<EntityID>(entityID);
            if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Text>(entity)) return;
            mgr.getComponent<EC_UI_Text>(entity).colour = glm::vec4(r, g, b, a);
        }

        void setUIPanelColour(unsigned int entityID, float r, float g, float b, float a) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = static_cast<EntityID>(entityID);
            if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Panel>(entity)) return;
            mgr.getComponent<EC_UI_Panel>(entity).colour = glm::vec4(r, g, b, a);
        }

        void setUIVisible(unsigned int entityID, bool visible) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = static_cast<EntityID>(entityID);
            if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
            mgr.getComponent<EC_UI_Element>(entity).visible = visible;
        }

        void setUIPosition(unsigned int entityID, float x, float y) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = static_cast<EntityID>(entityID);
            if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
            mgr.getComponent<EC_UI_Element>(entity).position = glm::vec2(x, y);
        }

        void setUISize(unsigned int entityID, float w, float h) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = static_cast<EntityID>(entityID);
            if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
            mgr.getComponent<EC_UI_Element>(entity).size = glm::vec2(w, h);
        }

        void setUILayer(unsigned int entityID, int layer) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = static_cast<EntityID>(entityID);
            if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
            mgr.getComponent<EC_UI_Element>(entity).layer = layer;
        }

        unsigned int createUIElement(float x, float y, float w, float h, int layer) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            EntityID entity = mgr.createEntity();
            EC_UI_Element element;
            element.position = glm::vec2(x, y);
            element.size = glm::vec2(w, h);
            element.layer = layer;
            mgr.addComponent(entity, element);
            return entity;
        }

        float getFPS() {
            if (!game) return 0.0f;
            return game->getFPS();
        }

        float getMSPF() {
            if (!game) return 0.0f;
            return game->getMSPF();
        }

        int getRecentLogCount() {
            return static_cast<int>(LOGGING::ECX_Logger::GetInstance()->GetRecentPlainLogs(200).size());
        }

        std::string getRecentLog(int index) {
            auto logs = LOGGING::ECX_Logger::GetInstance()->GetRecentPlainLogs(200);
            if (index < 0 || static_cast<size_t>(index) >= logs.size()) return "";
            return logs[index];
        }

        void setMouseCaptured(bool captured) {
            if (game) game->setMouseCaptured(captured);
        }

        void log(const std::string& message) {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(message, LOGGING::LogLevel::INFORMATION);
        }

        // Issue #30. Returns all entities the ray intersects (not just the nearest) unless
        // firstHitOnly is set. Caches the result for the paginated getters below - avoids
        // marshaling a vector-of-struct across the Lua boundary, matching the
        // getRecentLogCount/getRecentLog pattern already used for the debug overlay.
        int rayQuery(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance, bool firstHitOnly = false) {
            if (!game) { m_LastRayHits.clear(); return 0; }
            m_LastRayHits = game->queryRay(glm::vec3(ox, oy, oz), glm::vec3(dx, dy, dz), maxDistance, firstHitOnly);
            return static_cast<int>(m_LastRayHits.size());
        }

        EntityAPI getRayHitEntity(int index) {
            if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return EntityAPI(INVALID_ENTITY);
            return EntityAPI(m_LastRayHits[index].entity);
        }

        glm::vec3 getRayHitPosition(int index) {
            if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return glm::vec3(0.0f);
            return m_LastRayHits[index].position;
        }

        glm::vec3 getRayHitNormal(int index) {
            if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return glm::vec3(0.0f);
            return m_LastRayHits[index].normal;
        }

        float getRayHitDistance(int index) {
            if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return 0.0f;
            return m_LastRayHits[index].distance;
        }

        // Issue #29. Entities whose shape overlaps the cone, restricted to castsShadow ==
        // true geometry by default. checkOcclusion opts into additionally requiring
        // unobstructed line-of-sight to the apex (a candidate stacked behind a closer one
        // is excluded) - independent of containment, not fused into it. Same caching
        // pattern as rayQuery above.
        int coneQuery(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance, bool castsShadowOnly = true, bool checkOcclusion = false) {
            if (!game) { m_LastConeHits.clear(); return 0; }
            m_LastConeHits = game->queryCone(glm::vec3(ax, ay, az), glm::vec3(dx, dy, dz), halfAngleDegrees, maxDistance, castsShadowOnly, checkOcclusion);
            return static_cast<int>(m_LastConeHits.size());
        }

        EntityAPI getConeHitEntity(int index) {
            if (index < 0 || static_cast<size_t>(index) >= m_LastConeHits.size()) return EntityAPI(INVALID_ENTITY);
            return EntityAPI(m_LastConeHits[index].entity);
        }

        glm::vec3 getConeHitPosition(int index) {
            if (index < 0 || static_cast<size_t>(index) >= m_LastConeHits.size()) return glm::vec3(0.0f);
            return m_LastConeHits[index].position;
        }

        float getConeHitDistance(int index) {
            if (index < 0 || static_cast<size_t>(index) >= m_LastConeHits.size()) return 0.0f;
            return m_LastConeHits[index].distance;
        }

        // Visualizes the last ray/cone query (Issues #30/#29) - a debug draw only, no
        // effect on collision/query behaviour. Persists until replaced by another call.
        void showDebugRay(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance) {
            if (!messenger) return;
            ECXCommand cmd;
            cmd.type = ECXCommandType::GraphicsShowDebugRay;
            cmd.args[0] = DebugRayVisualization{ glm::vec3(ox, oy, oz), glm::normalize(glm::vec3(dx, dy, dz)), maxDistance };
            messenger->publish(cmd);
        }

        void showDebugCone(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance) {
            if (!messenger) return;
            ECXCommand cmd;
            cmd.type = ECXCommandType::GraphicsShowDebugCone;
            cmd.args[0] = DebugConeVisualization{ glm::vec3(ax, ay, az), glm::normalize(glm::vec3(dx, dy, dz)), glm::radians(halfAngleDegrees), maxDistance };
            messenger->publish(cmd);
        }
    private:
        std::vector<RayQueryHit> m_LastRayHits;
        std::vector<RayQueryHit> m_LastConeHits;
        void updateDepth(EntityID entity, uint32_t depth) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Hierarchy>(entity)) return;
            auto& hierarchy = mgr.getComponent<EC_DOD_Hierarchy>(entity);
            hierarchy.depth = depth;
            for (EntityID child : hierarchy.children)
                updateDepth(child, depth + 1);
        }
    };
}