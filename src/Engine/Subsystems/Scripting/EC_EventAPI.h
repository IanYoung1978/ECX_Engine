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
    };
}
