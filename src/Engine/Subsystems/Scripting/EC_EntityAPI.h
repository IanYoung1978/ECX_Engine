#pragma once
#include "Entity/EC_DOD_Types.h"
#include <glm/glm.hpp>
#include <string>

namespace ScriptAPI
{
    struct EntityAPI
    {
        EntityID entityID;

        EntityAPI(EntityID id) : entityID(id) {}

        float getBlendFactor();
        void setBlendFactor(float factor);
        std::string getName();
        unsigned int getUID();
        unsigned int getID();
        bool isActive();
        void activate();
        void deactivate();

        glm::vec3 getPosition();
        void setPosition(float x, float y, float z);
        glm::vec3 getVelocity();
        void setVelocity(float x, float y, float z);
        glm::vec3 getOrientation();
        void setOrientation(float x, float y, float z);
        glm::vec3 getAngularVelocity();
        void setAngularVelocity(float x, float y, float z);
        glm::vec3 getForward();
        glm::vec3 getUp();
        glm::vec3 getRight();

        void moveForward(float amount);
        void moveBack(float amount);
        void moveLeft(float amount);
        void moveRight(float amount);
        void moveUp(float amount);
        void moveDown(float amount);
        void rotateAroundAxis(float angle, float x, float y, float z);

        glm::vec4 getColour();
        void setColour(float r, float g, float b, float a = 1.0f);

        // Issue #98 - lets a reusable script (e.g. ArcadeGravity.lua) read an entity's own
        // authored collider dimensions back, instead of hand-keeping a separate hardcoded
        // copy in sync with whatever <Collider><Radius>/<Height> the entity was actually
        // given. 0 for an entity with no Collider component at all.
        float getColliderRadius();
        float getColliderHeight();

        // Hierarchy queries
        bool hasParent();
        unsigned int getParentID();
        unsigned int getDepth();

        // Script variables
        void setFloat(const std::string& name, float value);
        float getFloat(const std::string& name, float defaultVal = 0.0f);
        void setString(const std::string& name, const std::string& value);
        std::string getString(const std::string& name, const std::string& defaultVal = "");
    };
}
