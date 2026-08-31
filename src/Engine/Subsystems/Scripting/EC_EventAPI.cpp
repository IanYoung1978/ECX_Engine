#include "Engine/Subsystems/Scripting/EC_EventAPI.h"
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Messaging/KeyEvent.h"
#include "Messaging/MouseEvent.h"

namespace ScriptAPI
{
    std::string EventAPI::getKey() {
        if (event.type == ECXEventType::key_down ||
            event.type == ECXEventType::key_up ||
            event.type == ECXEventType::key_held) {
            try {
                KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                return keyEvent.getKeyString();
            }
            catch (const std::bad_any_cast&) { return ""; }
        }
        return "";
    }

    bool EventAPI::isPressed() {
        if (event.type == ECXEventType::key_down ||
            event.type == ECXEventType::key_up ||
            event.type == ECXEventType::key_held) {
            try {
                KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                return keyEvent.isPressed();
            }
            catch (const std::bad_any_cast&) { return false; }
        }
        return false;
    }

    bool EventAPI::isHeld() {
        if (event.type == ECXEventType::key_down ||
            event.type == ECXEventType::key_up ||
            event.type == ECXEventType::key_held) {
            try {
                KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                return keyEvent.isHeld();
            }
            catch (const std::bad_any_cast&) { return false; }
        }
        return false;
    }

    bool EventAPI::isReleased() {
        if (event.type == ECXEventType::key_down ||
            event.type == ECXEventType::key_up ||
            event.type == ECXEventType::key_held) {
            try {
                KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                return keyEvent.isReleased();
            }
            catch (const std::bad_any_cast&) { return false; }
        }
        return false;
    }

    float EventAPI::getMouseMotionX() {
        if (event.type == ECXEventType::mouse_move ||
            event.type == ECXEventType::mouse_down ||
            event.type == ECXEventType::mouse_up ||
            event.type == ECXEventType::mouse_held) {
            try {
                MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                return static_cast<float>(mouseEvent.getXMotion());
            }
            catch (const std::bad_any_cast&) { return 0.0f; }
        }
        return 0.0f;
    }

    float EventAPI::getMouseMotionY() {
        if (event.type == ECXEventType::mouse_move ||
            event.type == ECXEventType::mouse_down ||
            event.type == ECXEventType::mouse_up ||
            event.type == ECXEventType::mouse_held) {
            try {
                MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                return static_cast<float>(mouseEvent.getYMotion());
            }
            catch (const std::bad_any_cast&) { return 0.0f; }
        }
        return 0.0f;
    }

    int EventAPI::getMouseButton() {
        if (event.type == ECXEventType::mouse_down ||
            event.type == ECXEventType::mouse_up ||
            event.type == ECXEventType::mouse_held) {
            try {
                MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                return static_cast<int>(mouseEvent.getMouseKey());
            }
            catch (const std::bad_any_cast&) { return 0; }
        }
        return 0;
    }

    bool EventAPI::mouseButtonPressed() {
        if (event.type == ECXEventType::mouse_down) {
            try {
                MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                return mouseEvent.buttonPressed();
            }
            catch (const std::bad_any_cast&) { return false; }
        }
        return false;
    }

    bool EventAPI::mouseButtonHeld() {
        if (event.type == ECXEventType::mouse_held) {
            try {
                MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                return mouseEvent.buttonHeld();
            }
            catch (const std::bad_any_cast&) { return false; }
        }
        return false;
    }

    bool EventAPI::mouseButtonReleased() {
        if (event.type == ECXEventType::mouse_up) {
            try {
                MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                return mouseEvent.buttonReleased();
            }
            catch (const std::bad_any_cast&) { return false; }
        }
        return false;
    }

    glm::vec3 EventAPI::getNewPosition() {
        if (event.type == ECXEventType::EntityChangePosition) {
            try { return std::any_cast<glm::vec3>(event.args[0]); }
            catch (const std::bad_any_cast&) { return glm::vec3(0); }
        }
        return glm::vec3(0);
    }

    glm::vec3 EventAPI::getNewOrientation() {
        if (event.type == ECXEventType::EntityChangeOrientation) {
            try { return std::any_cast<glm::vec3>(event.args[0]); }
            catch (const std::bad_any_cast&) { return glm::vec3(0); }
        }
        return glm::vec3(0);
    }

    glm::vec3 EventAPI::getNewVelocity() {
        if (event.type == ECXEventType::EntityChangeVelocity) {
            try { return std::any_cast<glm::vec3>(event.args[0]); }
            catch (const std::bad_any_cast&) { return glm::vec3(0); }
        }
        return glm::vec3(0);
    }

    glm::vec3 EventAPI::getNewAngularVelocity() {
        if (event.type == ECXEventType::EntityChangeAngularVelocity) {
            try { return std::any_cast<glm::vec3>(event.args[0]); }
            catch (const std::bad_any_cast&) { return glm::vec3(0); }
        }
        return glm::vec3(0);
    }

    unsigned int EventAPI::entityIdToUID(unsigned int entityID) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return 0;
        if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return 0;
        return mgr.getComponent<EC_DOD_EntityInfo>(entityID).uid;
    }

    unsigned int EventAPI::getCollisionEntityA() {
        if (event.type == ECXEventType::CollisionBeginEvent) {
            try { return std::any_cast<unsigned int>(event.args[1]); }
            catch (const std::bad_any_cast&) { return 0; }
        }
        if (event.type == ECXEventType::CollisionEndEvent) {
            try { return std::any_cast<unsigned int>(event.args[0]); }
            catch (const std::bad_any_cast&) { return 0; }
        }
        return 0;
    }

    unsigned int EventAPI::getCollisionEntityB() {
        if (event.type == ECXEventType::CollisionBeginEvent) {
            try { return std::any_cast<unsigned int>(event.args[2]); }
            catch (const std::bad_any_cast&) { return 0; }
        }
        if (event.type == ECXEventType::CollisionEndEvent) {
            try { return std::any_cast<unsigned int>(event.args[1]); }
            catch (const std::bad_any_cast&) { return 0; }
        }
        return 0;
    }

    unsigned int EventAPI::getOtherEntityID() {
        if (event.type == ECXEventType::CollisionBeginEvent ||
            event.type == ECXEventType::CollisionEndEvent) {
            try {
                const unsigned int entityA = std::any_cast<unsigned int>(event.args[1]);
                const unsigned int entityB = std::any_cast<unsigned int>(event.args[2]);
                if (currentEntityID == entityA) return entityB;
                if (currentEntityID == entityB) return entityA;
                return 0;
            }
            catch (const std::bad_any_cast&) { return 0; }
        }
        return 0;
    }
}
