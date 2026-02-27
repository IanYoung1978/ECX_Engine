#pragma once
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Entity/EC_DOD_Types.h"
#include "Engine/Subsystems/Scripting/EC_EntityAPI.h"
#include <string>
#include <algorithm>

class EC_Game;

namespace ScriptAPI
{
    struct GameAPI
    {
        EC_Game* game;

        GameAPI(EC_Game* g) : game(g) {}

        EntityAPI getEntityByName(const std::string& name) {
            if (!game) return EntityAPI(INVALID_ENTITY);
            return EntityAPI(game->getEntityByName(name));
        }

        unsigned int getEntityIDByUID(unsigned int uid) {
            if (!game) return INVALID_ENTITY;
            return game->getEntityByUID(static_cast<uint32_t>(uid));
        }
        void toggleDebug() {
			LOGGING::ECX_Logger::GetInstance()->LogMessage("Toggling debug mode", LOGGING::LogLevel::INFORMATION);
            if (game) game->toggleDebug();
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