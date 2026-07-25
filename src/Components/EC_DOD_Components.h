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