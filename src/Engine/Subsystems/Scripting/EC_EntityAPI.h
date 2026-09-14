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

        // Issue #129. Camera (EC_DOD_Camera) - no-op / 0 on an entity with no Camera
        // component, same convention as getColliderRadius/Height above.
        float getFOV();
        void setFOV(float degrees);
        float getNearPlane();
        void setNearPlane(float distance);
        float getFarPlane();
        void setFarPlane(float distance);
        // isActive here is exactly the flag GL_Deferred_Renderer already checks at every
        // render call site to decide which camera(s) to draw from - switching the active
        // camera from script (cutscene cuts, security-cam mechanics) is just flipping this
        // on the new camera and off the old one, no separate engine-level switch needed.
        bool isCameraActive();
        void setCameraActive(bool active);

        // Issue #129. Lights (EC_DOD_Light). direction is a separate field from
        // orientation/getForward above - see the component's own comment - so spotlight
        // sweeps need this rather than the regular orientation setter.
        glm::vec3 getLightColour();
        void setLightColour(float r, float g, float b);
        float getLightIntensity();
        void setLightIntensity(float intensity);
        glm::vec3 getLightDirection();
        void setLightDirection(float x, float y, float z);
        bool getLightCastsShadow();
        void setLightCastsShadow(bool castsShadow);

        // Issue #129. Collider (EC_DOD_Collider) - center/extents beyond the existing
        // radius/height, plus runtime layer/mask for phasing/ghost-mode/"bullet ignores
        // shooter" patterns that today can only be authored once in XML.
        glm::vec3 getColliderCenter();
        void setColliderCenter(float x, float y, float z);
        glm::vec3 getColliderExtents();
        void setColliderExtents(float x, float y, float z);
        unsigned int getCollisionLayer();
        void setCollisionLayer(unsigned int layer);
        unsigned int getCollisionMask();
        void setCollisionMask(unsigned int mask);

        // Issue #129. GraphicsData (EC_DOD_GraphicsData) - visible is a pure render toggle,
        // distinct from activate()/deactivate() above (which also gates
        // physics/collision/logic via EC_DOD_EntityInfo::active).
        bool isVisible();
        void setVisible(bool visible);
        float getEmissiveIntensity();
        void setEmissiveIntensity(float intensity);
        bool getCastsShadow();
        void setCastsShadow(bool castsShadow);
        bool getReceivesShadow();
        void setReceivesShadow(bool receivesShadow);

        // Issue #129. Hierarchy (EC_DOD_Hierarchy) - hasParent/getParentID/getDepth above
        // only let a script walk UP the hierarchy; these walk down.
        unsigned int getChildCount();
        unsigned int getChildID(unsigned int index);

        // Issue #129. Skybox (EC_DOD_Skybox) - see the component's own comment: this is the
        // only way to align the HDR panorama's baked-in sun with a scene's actual
        // directional light direction (e.g. a script-driven day/night cycle) without
        // re-exporting the HDR asset.
        float getSkyboxRotation();
        void setSkyboxRotation(float degrees);

        // Issue #129. ScriptData (EC_DOD_ScriptData::enabled) - suppresses this entity's own
        // event handlers (OnUpdate/OnKeyDown/etc, see EC_LuaScriptSystem) without touching
        // activate()/deactivate() above, e.g. a stunned/frozen state that should keep
        // rendering/colliding but stop reacting to input.
        bool isScriptEnabled();
        void setScriptEnabled(bool enabled);

        // Issue #130. Persistent, script-set external force (EC_DOD_ExternalForce) -
        // integrated into velocity every physics substep exactly like gravity, until
        // changed again (call with 0,0,0 to turn it off) - the standard "thruster" idiom.
        // No-op on an entity with no RigidBody. See that component's own comment for why
        // this must persist rather than clear after one substep.
        void applyForce(float x, float y, float z);

        // Issue #130. One-shot instantaneous change, added directly to velocity/
        // angular velocity this frame - the standard recoil/knockback/jump-pad/explosion
        // idiom. No-op on an entity with no RigidBody (or a static one).
        void applyImpulse(float x, float y, float z);
        void applyTorque(float x, float y, float z);

        // Issue #130. Runtime RigidBody (EC_DOD_RigidBody) property access - authoring-
        // time-only (XML <RigidBody>) until now. 0/false on an entity with no RigidBody.
        float getMass();
        void setMass(float mass);
        float getRestitution();
        void setRestitution(float restitution);
        float getFriction();
        void setFriction(float friction);
        float getStaticFriction();
        void setStaticFriction(float staticFriction);
        bool isStatic();
        void setStatic(bool isStatic);
        bool isSleeping();
        void wake();
    };
}
