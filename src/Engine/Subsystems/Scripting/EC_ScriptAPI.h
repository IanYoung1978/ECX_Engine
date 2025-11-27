#pragma once
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Messaging/ECXEvent.h"
#include "Messaging/KeyEvent.h"
#include "Messaging/MouseEvent.h"
#include <glm/glm.hpp>
#include <string>

// Forward declaration
class EC_Game;

namespace ScriptAPI
{
    // Entity API for Lua scripts - now works with EntityID
    struct EntityAPI {
        EntityID entityID;

        EntityAPI(EntityID id) : entityID(id) {}

        std::string getName() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return "";
            return mgr.getComponent<EC_DOD_EntityInfo>(entityID).name;
        }

        unsigned int getUID() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return 0;
            return mgr.getComponent<EC_DOD_EntityInfo>(entityID).uid;
        }

        bool isActive() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return false;
            return mgr.getComponent<EC_DOD_EntityInfo>(entityID).active;
        }

        void activate() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return;
            auto& info = mgr.getComponent<EC_DOD_EntityInfo>(entityID);
            info.active = true;
        }

        void deactivate() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return;
            auto& info = mgr.getComponent<EC_DOD_EntityInfo>(entityID);
            info.active = false;
        }

        // Spatial component access
        glm::vec3 getPosition() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).position;
        }

        void setPosition(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.position = glm::vec3(x, y, z);
        }

        glm::vec3 getVelocity() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).velocity;
        }

        void setVelocity(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.velocity = glm::vec3(x, y, z);
        }

        glm::vec3 getOrientation() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).orientation;
        }

        void setOrientation(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.orientation = glm::vec3(x, y, z);
        }

        glm::vec3 getAngularVelocity() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).angVelocity;
        }

        void setAngularVelocity(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.angVelocity = glm::vec3(x, y, z);
        }

        glm::vec3 getForward() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0, 0, -1);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).direction;
        }

        glm::vec3 getUp() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0, 1, 0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).up;
        }

        glm::vec3 getRight() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(1, 0, 0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).right;
        }

        // Convenience movement functions
        void moveForward(float amount) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.position += spatial.direction * amount;
        }

        void moveBack(float amount) {
            moveForward(-amount);
        }

        void moveLeft(float amount) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.position -= spatial.right * amount;
        }

        void moveRight(float amount) {
            moveLeft(-amount);
        }

        void moveUp(float amount) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.position += spatial.up * amount;
        }

        void moveDown(float amount) {
            moveUp(-amount);
        }

        void rotateAroundAxis(float angle, float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            glm::vec3 axis(x, y, z);
            spatial.orientation += axis * angle;
        }

        // Script variables (stored in EC_DOD_ScriptData)
        void setFloat(const std::string& name, float value) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return;
            auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
            script.floatVars[name] = value;
        }

        float getFloat(const std::string& name, float defaultVal = 0.0f) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return defaultVal;
            auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
            auto it = script.floatVars.find(name);
            return (it != script.floatVars.end()) ? it->second : defaultVal;
        }

        void setString(const std::string& name, const std::string& value) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return;
            auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
            script.stringVars[name] = value;
        }

        std::string getString(const std::string& name, const std::string& defaultVal = "") {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return defaultVal;
            auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
            auto it = script.stringVars.find(name);
            return (it != script.stringVars.end()) ? it->second : defaultVal;
        }
    };

    struct GameAPI {
        EC_Game* game;
        GameAPI(EC_Game* g) : game(g) {}

        // TODO: You'll need to add a method to EC_Game to get entity by name
        // For now, returning invalid entity
        EntityAPI getEntity(const std::string& name) {
            // You need to implement getEntityByName that returns EntityID in EC_Game
            // For now:
            return EntityAPI(INVALID_ENTITY);
        }

        void shutdown() {
            if (game) game->shutDown();
        }

        int getKeyState(const std::string& key) {
            if (game) {
                SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());
                KeyState state = game->getKeyState(scancode);
                return static_cast<int>(state);
            }
            return static_cast<int>(KeyState::None);
        }
    };

    // Event API remains mostly the same
    struct EventAPI {
        ECXEvent& event;
        EC_Game* game;

        EventAPI(ECXEvent& e, EC_Game* g) : event(e), game(g) {}

        std::string getKey() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.getKeyString();
                }
                catch (const std::bad_any_cast&) {
                    return "";
                }
            }
            return "";
        }

        bool isPressed() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.isPressed();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool isHeld() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.isHeld();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool isReleased() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.isReleased();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        float getMouseMotionX() {
            if (event.type == ECXEventType::mouse_move ||
                event.type == ECXEventType::mouse_down ||
                event.type == ECXEventType::mouse_up ||
                event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return static_cast<float>(mouseEvent.getXMotion());
                }
                catch (const std::bad_any_cast&) {
                    return 0.0f;
                }
            }
            return 0.0f;
        }

        float getMouseMotionY() {
            if (event.type == ECXEventType::mouse_move ||
                event.type == ECXEventType::mouse_down ||
                event.type == ECXEventType::mouse_up ||
                event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return static_cast<float>(mouseEvent.getYMotion());
                }
                catch (const std::bad_any_cast&) {
                    return 0.0f;
                }
            }
            return 0.0f;
        }

        int getMouseButton() {
            if (event.type == ECXEventType::mouse_down ||
                event.type == ECXEventType::mouse_up ||
                event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return static_cast<int>(mouseEvent.getMouseKey());
                }
                catch (const std::bad_any_cast&) {
                    return 0;
                }
            }
            return 0;
        }

        bool mouseButtonPressed() {
            if (event.type == ECXEventType::mouse_down) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return mouseEvent.buttonPressed();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool mouseButtonHeld() {
            if (event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return mouseEvent.buttonHeld();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool mouseButtonReleased() {
            if (event.type == ECXEventType::mouse_up) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return mouseEvent.buttonReleased();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        unsigned int getOtherEntityUID() {
            if (event.type == ECXEventType::CollisionBeginEvent ||
                event.type == ECXEventType::CollisionEndEvent) {
                try {
                    return std::any_cast<unsigned int>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return 0;
                }
            }
            return 0;
        }

        glm::vec3 getNewPosition() {
            if (event.type == ECXEventType::EntityChangePosition) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }

        glm::vec3 getNewOrientation() {
            if (event.type == ECXEventType::EntityChangeOrientation) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }

        glm::vec3 getNewVelocity() {
            if (event.type == ECXEventType::EntityChangeVelocity) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }

        glm::vec3 getNewAngularVelocity() {
            if (event.type == ECXEventType::EntityChangeAngularVelocity) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }
    };
}