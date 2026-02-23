#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include "Entity/EC_DOD_EntityManager.h"
#include "Graphics/TextureSet.h"
#include "Graphics/ObjModel.h"
#include "Graphics/Shader.h"


class Shader;
class ObjModel;

struct ContactPoint {
    glm::vec3 position;        // World space contact position
    glm::vec3 normal;          // Contact normal (points from A to B)
    float penetration;         // Penetration depth
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

    // Shape parameters
    glm::vec3 center{ 0.0f };        // Local offset from entity position
    glm::vec3 extents{ 1.0f };       // Half-extents for box/AABB
    float radius = 1.0f;              // For sphere/capsule/cylinder
    float height = 2.0f;              // For capsule/cylinder

    // Collision filtering
    uint32_t collisionLayer = 1;     // What layer is this on?
    uint32_t collisionMask = 0xFFFFFFFF;  // What layers can it collide with?
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

struct EC_DOD_Transform {
    glm::mat4 localTransform{ 1.0f };
    glm::mat4 worldTransform{ 1.0f };
    bool dirty = true;
};

struct EC_DOD_Hierarchy {
    EntityID parent = INVALID_ENTITY;
    std::vector<EntityID> children;
};

struct EC_DOD_GraphicsData {
    // Store names for post-load resolution
    std::string modelName;
    std::string vertShader;
    std::string fragShader;

    // Actual resources (set in performPostLoadActions)
    std::shared_ptr<TextureSet> textureSet;
    std::shared_ptr<ObjModel> model;
    std::shared_ptr<Shader> shader;

    glm::vec4 colour{ 1.0f };
    bool hasTextures = false;
    bool visible = true;

    // Helper methods
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
    float range;
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
    std::string scriptFile;
    bool enabled = true;
    // Per-entity persistent variables (accessible from Lua)
    // These survive between frames - perfect for entity state
    std::unordered_map<std::string, float> floatVars;
    std::unordered_map<std::string, std::string> stringVars;
};