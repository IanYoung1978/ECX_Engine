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

        // The entity's authored collider radius/height; 0 if it has no Collider.
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

        // Camera: no-op / 0 on an entity with no Camera.
        float getFOV();
        void setFOV(float degrees);
        float getNearPlane();
        void setNearPlane(float distance);
        float getFarPlane();
        void setFarPlane(float distance);
        // isActive is the flag the renderer checks to choose which cameras to draw from; switch cameras by
        // setting it on the new one and off the old one.
        bool isCameraActive();
        void setCameraActive(bool active);

        // Lights. direction is separate from orientation/getForward (see the component), so spotlight
        // sweeps need this setter.
        glm::vec3 getLightColour();
        void setLightColour(float r, float g, float b);
        float getLightIntensity();
        void setLightIntensity(float intensity);
        glm::vec3 getLightDirection();
        void setLightDirection(float x, float y, float z);
        bool getLightCastsShadow();
        void setLightCastsShadow(bool castsShadow);

        // Collider: center/extents beyond radius/height, plus runtime layer/mask (phasing, ghost mode).
        glm::vec3 getColliderCenter();
        void setColliderCenter(float x, float y, float z);
        glm::vec3 getColliderExtents();
        void setColliderExtents(float x, float y, float z);
        unsigned int getCollisionLayer();
        void setCollisionLayer(unsigned int layer);
        unsigned int getCollisionMask();
        void setCollisionMask(unsigned int mask);

        // GraphicsData: visible is a pure render toggle, unlike activate()/deactivate(), which also gate
        // physics, collision and logic.
        bool isVisible();
        void setVisible(bool visible);
        float getEmissiveIntensity();
        void setEmissiveIntensity(float intensity);
        bool getCastsShadow();
        void setCastsShadow(bool castsShadow);
        bool getReceivesShadow();
        void setReceivesShadow(bool receivesShadow);

        // Hierarchy: walk down (the getters above only walk up).
        unsigned int getChildCount();
        unsigned int getChildID(unsigned int index);

        // Skybox: aligns the HDR panorama's baked-in sun with a scene's directional light (e.g. a
        // day/night cycle) without re-exporting the asset.
        float getSkyboxRotation();
        void setSkyboxRotation(float degrees);

        // ScriptData::enabled suppresses this entity's own event handlers without touching
        // activate()/deactivate() (e.g. a stunned entity that keeps rendering and colliding).
        bool isScriptEnabled();
        void setScriptEnabled(bool enabled);

        // Persistent external force, integrated every physics substep like gravity until changed
        // (0,0,0 turns it off). No-op on an entity with no RigidBody.
        void applyForce(float x, float y, float z);

        // One-shot change added directly to velocity/angular velocity (recoil, knockback). No-op on an
        // entity with no RigidBody or a static one.
        void applyImpulse(float x, float y, float z);
        void applyTorque(float x, float y, float z);

        // Runtime RigidBody property access; 0/false on an entity with no RigidBody.
        float getMass();
        void setMass(float mass);
        float getRestitution();
        void setRestitution(float restitution);
        float getFriction();
        void setFriction(float friction);
        float getStaticFriction();
        void setStaticFriction(float staticFriction);
        // Static/Kinematic/Dynamic (see EC_BodyType); getBodyType/setBodyType use the XML strings.
        // setStatic(false) and setKinematic(false) both mean Dynamic. "" / no-op on an entity with no
        // RigidBody.
        std::string getBodyType();
        void setBodyType(const std::string& bodyType);
        bool isStatic();
        void setStatic(bool isStatic);
        bool isKinematic();
        void setKinematic(bool isKinematic);
        bool isSleeping();
        void wake();

        // Runtime toggle, typically set by an OnCollisionBegin handler (see EC_DOD_RigidBody::ignoreGravity).
        // false on an entity with no RigidBody.
        bool getIgnoreGravity();
        void setIgnoreGravity(bool ignore);
    };
}
