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

        // CollisionBeginEvent-only (CollisionEndEvent carries no manifold - see
        // EC_NarrowPhase.cpp's own publish calls). Zero-valued default on any other event
        // type. See EC_CollisionShapes.h's CollisionManifold for what these mean - the
        // normal generally points body_A -> body_B (EC_NarrowPhase's dispatch table's own
        // convention), except a Mesh-involving pair where it's the triangle's own absolute
        // surface normal instead (see EC_CollisionChecks::CapsuleVsMesh's own comment).
        glm::vec3 getCollisionNormal();
        float getCollisionPenetrationDepth();
    };
}
