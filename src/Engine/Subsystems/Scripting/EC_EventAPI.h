#pragma once
#include "Messaging/ECXEvent.h"
#include "Entity/EC_DOD_Types.h"
#include <glm/glm.hpp>
#include <string>

class EC_Game;

namespace ScriptAPI
{
    struct EventAPI
    {
        ECXEvent& event;
        EC_Game* game;
        EntityID currentEntityID;

        EventAPI(ECXEvent& e, EC_Game* g, EntityID currentEntity)
            : event(e), game(g), currentEntityID(currentEntity) {
        }

        std::string getKey();
        bool isPressed();
        bool isHeld();
        bool isReleased();
        // Real per-frame delta time captured when this event was published - only
        // meaningful for key_held (see KeyEvent::getDeltaTime()'s own comment); 0.0 for
        // every other event type, including key_down/key_up.
        float getDeltaTime();

        float getMouseMotionX();
        float getMouseMotionY();
        int getMouseButton();
        bool mouseButtonPressed();
        bool mouseButtonHeld();
        bool mouseButtonReleased();

        glm::vec3 getNewPosition();
        glm::vec3 getNewOrientation();
        glm::vec3 getNewVelocity();
        glm::vec3 getNewAngularVelocity();

        unsigned int entityIdToUID(unsigned int entityID);
        unsigned int getCollisionEntityA();
        unsigned int getCollisionEntityB();
        unsigned int getOtherEntityID();

        // CollisionBeginEvent only (End carries no manifold); zero on any other event. The normal generally
        // points body_A -> body_B, except for Mesh pairs where it's the triangle's absolute surface normal
        // (see EC_CollisionChecks::CapsuleVsMesh).
        glm::vec3 getCollisionNormal();
        float getCollisionPenetrationDepth();
    };
}
