#pragma once
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Entity/EC_DOD_Types.h"
#include "Engine/Subsystems/Scripting/EC_EntityAPI.h"
#include "UI/EC_UI_Components.h"
#include "Logging/ECX_Logging.h"
#include <string>
#include <algorithm>
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
    private:
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