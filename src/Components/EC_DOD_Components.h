#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <memory>
#include <vector>
#include "Entity/EC_DOD_EntityManager.h"
#include "Graphics/Textures/TextureSet.h"
#include "Graphics/Shaders/Shader.h"
#include "Messaging/ECXEventType.h"

class Shader;
class ObjModel;

struct ContactPoint {
    glm::vec3 position;
    glm::vec3 normal;
    float penetration;
};

// Published per physics tick by EC_PhysicsSystem so debug rendering can draw contact points through
// the normal component-array path, without reading EC_PairManager (physics-thread-internal).
struct EC_DOD_DebugContacts {
    std::vector<glm::vec3> points;
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
        // Real ray/cone testing against EC_DOD_MeshCollisionData's actual triangles,
        // rather than any convex-primitive approximation - see that component's own
        // comment. `center`/`extents` still describe this collider's broad-phase bounding
        // box exactly as AABB's do; only the precise per-candidate test differs.
        Mesh,
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

// Triangle-mesh geometry for Collider::Type::Mesh entities (today only EC_VoxelChunkSystem
// populates it, keeping the mesher output because ObjModel exposes none). shared_ptr so copying this
// component is a refcount bump, not a mesh copy.
struct EC_DOD_MeshCollisionData {
    std::shared_ptr<std::vector<glm::vec3>> positions;
    std::shared_ptr<std::vector<glm::vec3>> normals;
    std::shared_ptr<std::vector<uint32_t>> indices;
};

struct EC_DOD_Spatial {
    glm::vec3 position{ 0.0f };
    glm::vec3 velocity{ 0.0f };
    glm::vec3 orientation{ 0.0f };
    glm::vec3 angVelocity{ 0.0f };
    glm::vec3 direction{ 0.0f, 0.0f, -1.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };
    // 3-axis rotation state for rigid bodies (see EC_PhysicsSystem's integration); orientation/right/
    // up/direction are kept in sync from it each physics tick. Entities without a RigidBody never touch it.
    glm::quat orientationQuat{ 1.0f, 0.0f, 0.0f, 0.0f };
};

// Static never moves. Kinematic is moved by scripts: the impulse solver skips it, but it still
// gets collision events (and gravity unless ignoreGravity). Dynamic is fully simulated (default).
enum class EC_BodyType : uint8_t {
    Static,
    Kinematic,
    Dynamic
};

struct EC_DOD_RigidBody {
    float mass = 1.0f;
    float restitution = 0.3f;
    // Kinetic (sliding) friction coefficient - caps the friction impulse
    // once a contact is actually slipping. See staticFriction below for the
    // separate, normally-higher threshold that governs whether it starts
    // slipping in the first place.
    float friction = 0.5f;
    // Static friction coefficient: the tangential impulse a contact must exceed before it slips
    // (usually >= friction: it takes more force to start sliding than to keep going). Defaults to
    // matching friction.
    float staticFriction = 0.5f;
    // Rolling resistance: a small angular impulse at each contact opposing rolling, clamped to this
    // times the normal impulse. Keep it well below friction or it cancels real topples.
    float rollingFriction = 0.01f;
    // Per-second fractional velocity decay (0 = none), applied every frame
    // regardless of contact - models drag/internal energy loss. Distinct
    // from friction, which only acts at a contact point.
    float linearDamping = 0.01f;
    float angularDamping = 0.05f;
    EC_BodyType bodyType = EC_BodyType::Dynamic;
    // EC_PhysicsSystem's gravity step skips entities with this set. Usually toggled by script (on
    // landing and on jump) but can be authored. Independent of bodyType: a Kinematic body may still want
    // gravity.
    bool ignoreGravity = false;
    // Runtime state, not authored: once velocity has been negligible long enough the body is frozen (no
    // gravity, integration or resolution) until something with real velocity wakes it. Stops resting
    // bodies drifting from tiny numerical impulses.
    bool isSleeping = false;
    float sleepTimer = 0.0f;
};

// A persistent, script-set force (thrusters, wind), integrated into velocity every physics
// substep like gravity - not a one-shot impulse. Added by EntityAPI::applyForce(); stays
// until applyForce() is called again (0,0,0 turns it off).
struct EC_DOD_ExternalForce {
    glm::vec3 force{ 0.0f };
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
    // Defined out-of-line in EC_DOD_Components.cpp so this header needs only a forward declaration of
    // ObjModel; ECX_UnitTests includes it and must stay free of the engine's assimp/GL toolchain.
    uint32_t getMeshHandle() const;
    uint32_t getVertexCount() const;
};

// A voxel terrain chunk entity - the real, permanent integration (see
// EC_VoxelChunkSystem/EC_VoxelChunkWorker), not debug scaffolding. `state` tracks the
// async generate-then-upload lifecycle: Pending (spawned, not yet queued for generation),
// Generating (queued/running on the worker thread), Ready (mesh uploaded, collider added).
struct EC_DOD_VoxelChunk {
    enum class State { Pending, Generating, Ready };
    glm::ivec3 chunkCoord{ 0 };
    State state = State::Pending;
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
    Type type = Type::Point;
    glm::vec3 position{ 0.0f };
    glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
    glm::vec3 colour{ 1.0f };
    float intensity = 1.0f;
    // Distance beyond which this light's contribution falls below a perceptible
    // threshold (1/256, the smallest step an 8-bit colour channel can represent).
    // Computed once at load time (see EC_DOD_EntityFactory::parseLight) from
    // intensity/attenuation - not meaningful for Directional lights (left at 0).
    float cutoffRadius = 0.0f;
    float cutoffAngle = glm::radians(45.0f);
    // Constant-only by default (no linear/quadratic falloff) - this is exactly
    // the degenerate case computeLightCutoffRadius already falls back to a
    // fixed radius for, so an XML entry that omits <Attenuation> gets that
    // designed fallback instead of reading garbage.
    glm::vec3 attenuation{ 1.0f, 0.0f, 0.0f };
    bool castsShadow = false;
    bool dynamic = false;
};

// Scene-membership lifecycle, separate from EntityInfo::active (a script's on/off toggle). Only
// Active entities take part in per-frame queries. MarkedForDeletion is one-way.
enum class EC_SceneLifecycleState : uint8_t {
    Active,
    Inactive,
    MarkedForDeletion
};

struct EC_DOD_EntityInfo {
    std::string name;
    // Gameplay-level toggle - what EntityAPI::activate()/deactivate() (Lua-exposed) and
    // isActive() control. A script (e.g. a debug light-cycling feature) sets this
    // deliberately per entity and expects it to stick.
    bool active = true;
    // Scene-membership state, kept separate from `active` so switching scenes doesn't overwrite a
    // script's own on/off state. Subsystems must check active && sceneState == Active (see
    // EC_DOD_EntityManager::getActiveEntitiesWithComponents).
    EC_SceneLifecycleState sceneState = EC_SceneLifecycleState::Active;
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
    // Degrees, rotation about world Y - the HDR panorama's own baked-in sun position is
    // fixed at load time, so this is the only way to align it with a scene's actual
    // directional light direction (e.g. after changing the light to a low, near-horizon
    // angle for long shadows) without re-exporting the HDR asset itself.
    float rotationYDegrees = 0.0f;
};