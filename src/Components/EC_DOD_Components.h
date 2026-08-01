#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include "Entity/EC_DOD_EntityManager.h"
#include "Graphics/TextureSet.h"
#include "Graphics/ObjModel.h"
#include "Graphics/Shader.h"
#include "Messaging/ECXEventType.h"

class Shader;
class ObjModel;

struct ContactPoint {
    glm::vec3 position;
    glm::vec3 normal;
    float penetration;
};

struct EC_DOD_Hierarchy {
    EntityID parent = INVALID_ENTITY;
    std::vector<EntityID> children;
    uint32_t depth = 0;
};

struct EC_DOD_Collider {
    enum class Type : uint8_t {
        Sphere,
        AABB,
        OBB,
        Capsule,
        Cylinder,
        Frustum,
        Plane,
        None
    };
    Type type = Type::OBB;
    glm::vec3 center{ 0.0f };
    glm::vec3 extents{ 1.0f };
    float radius = 1.0f;
    float height = 2.0f;
    uint32_t collisionLayer = 1;
    uint32_t collisionMask = 0xFFFFFFFF;
};

struct EC_DOD_Spatial {
    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    glm::vec3 orientation{ 0.0f };
    glm::vec3 angVelocity{ 0.0f };
    glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };
};

struct EC_DOD_RigidBody {
    float mass = 1.0f;
    float restitution = 0.3f;
    // Kinetic (sliding) friction coefficient - caps the friction impulse
    // once a contact is actually slipping. See staticFriction below for the
    // separate, normally-higher threshold that governs whether it starts
    // slipping in the first place.
    float friction = 0.5f;
    // Static friction coefficient - the threshold a contact's tangential
    // impulse must exceed before it's allowed to slip at all (real
    // materials almost always have staticFriction >= friction; e.g. wood on
    // wood is roughly 0.5 static vs 0.3 kinetic - it takes more force to
    // START a resting object sliding than to KEEP it sliding). Defaults to
    // matching friction (no distinct static regime) for any material/entity
    // that doesn't specify one explicitly.
    float staticFriction = 0.5f;
    // Rolling resistance coefficient - a small torque, applied AT each
    // contact point, that opposes ROLLING (not sliding) motion there.
    // Ordinary Coulomb friction (friction/staticFriction above) only
    // resists relative TANGENTIAL SLIP at a contact point; a box balanced
    // on an edge, or a ball/cylinder, can ROTATE about that contact line
    // with zero slip there at all (identical to a wheel rolling without
    // slipping), so sliding friction alone can never stop it - the object
    // just keeps slowly rolling/rocking indefinitely, limited only by the
    // crude global angularDamping above, which can take many seconds to
    // arrest a small residual spin. Real materials dissipate this via
    // contact-patch deformation, modelled here the same way Bullet's
    // rollingFriction/spinningFriction do: a small angular impulse at the
    // contact, clamped by this coefficient times the normal impulse. Keep
    // this an order of magnitude below friction/staticFriction - it's
    // evaluated fresh against whatever angular velocity exists at the
    // START of each tick, so a coefficient anywhere near sliding-friction
    // scale can fully cancel a genuine gravity-driven topple's angular
    // velocity increment before it has a chance to build up (the impulse
    // needed to arrest a SMALL, just-starting angular velocity is itself
    // small, well within a large coefficient's budget against a loaded
    // contact's normal impulse) - that reads as an obviously-unstable
    // overhang crawling over several real seconds instead of toppling in
    // a fraction of one. Defaults to a small nonzero value rather than 0
    // since 0 reproduces the original (frictionless-rolling) behaviour
    // exactly.
    float rollingFriction = 0.01f;
    // Per-second fractional velocity decay (0 = none), applied every frame
    // regardless of contact - models drag/internal energy loss. Distinct
    // from friction, which only acts at a contact point.
    float linearDamping = 0.01f;
    float angularDamping = 0.05f;
    bool isStatic = false;
    // Runtime state (not authored via XML) - once velocity has been
    // negligible for long enough, the body is frozen entirely (no gravity,
    // no integration, no collision resolution) until something with real
    // velocity hits it and wakes it back up. Without this, resting bodies
    // keep receiving tiny numerical impulses every frame forever, which
    // shows up as slow drift/sinking over time even when otherwise at rest.
    bool isSleeping = false;
    float sleepTimer = 0.0f;
};

// Per-body running total of this frame's momentum changes from every source
// (gravity, every contact point across every colliding pair) - added to but
// never directly applied to EC_DOD_Spatial until the single, unified apply
// step (EC_PhysicsResolution::applyAccumulatedImpulses), which adds the
// total to velocity (linear) then angVelocity (angular) in one place, once
// per frame. Momentum units throughout (impulse = delta-momentum), so
// contributions from different sources/contact points are simply additive.
struct EC_DOD_ImpulseAccumulator {
    glm::vec3 deltaLinearMomentum{ 0.0f };
    glm::vec3 deltaAngularMomentum{ 0.0f };
};

struct EC_DOD_Transform {
    glm::mat4 matrix{ 1.0f };
    glm::vec3 scale{ 1.0f };
    bool dirty = true;
};

struct EC_DOD_GraphicsData {
    std::string modelName;
    std::string vertShader;
    std::string fragShader;
    std::shared_ptr<TextureSet> textureSet;
    std::shared_ptr<ObjModel> model;
    std::shared_ptr<Shader> shader;
    glm::vec4 colour{ 1.0f };
    bool hasTextures = false;
    bool visible = true;
    bool castsShadow = true;
    bool receivesShadow = true;
    float emissiveIntensity = 1.0f;
    uint32_t getMeshHandle() const { return model ? model->getHandle() : 0; }
    uint32_t getVertexCount() const { return model ? model->getVertCount() : 0; }
};

struct EC_DOD_Camera {
    glm::mat4 viewMatrix{ 1.0f };
    glm::mat4 projectionMatrix{ 1.0f };
    float fov = 60.0f;
    float nearPlane = 1.0f;
    float farPlane = 100.0f;
    bool isActive = true;
};

struct EC_DOD_Light {
    enum class Type : uint8_t {
        Directional,
        Point,
        Spot
    };
    Type type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 colour;
    float intensity;
    // Distance beyond which this light's contribution falls below a perceptible
    // threshold (1/256, the smallest step an 8-bit colour channel can represent).
    // Computed once at load time (see EC_DOD_EntityFactory::parseLight) from
    // intensity/attenuation - not meaningful for Directional lights (left at 0).
    float cutoffRadius;
    float cutoffAngle;
    glm::vec3 attenuation;
    bool castsShadow;
    bool dynamic;
};

struct EC_DOD_EntityInfo {
    std::string name;
    bool active = true;
    uint32_t uid = 0;
};

struct EC_DOD_ScriptData {
    bool enabled = true;
    std::unordered_map<ECXEventType, std::string> handlers;
    std::unordered_map<std::string, float> floatVars;
    std::unordered_map<std::string, std::string> stringVars;
};

struct EC_DOD_Skybox {
    std::string hdrPath;
    std::string targetHdrPath;
    unsigned int cubemapHandle = 0;
    unsigned int targetCubemapHandle = 0;
    float blendFactor = 1.0f;
};