#pragma once
#include "xml/tinyxml.h"
#include "EC_DOD_EntityManager.h"
#include "Graphics/TextureManager.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/MeshManager.h"
#include "Graphics/CubemapManager.h"
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include "EC_DOD_Types.h"
#include "Messaging/ECXEventType.h"

class EC_DOD_EntityFactory {
public:
    EC_DOD_EntityFactory();
    virtual ~EC_DOD_EntityFactory();
    EntityID constructEntity(TiXmlElement& descriptor);
    EntityID constructCamera(TiXmlElement& descriptor);
    void performPostLoadActions();
    static void finalizePendingGraphics(size_t maxPerCall);
    const std::vector<EntityID>& getCameras() const;
    const std::vector<EntityID>& getEntities() const;
    const std::vector<EntityID>& getLights() const;

    static void parseManifest(TiXmlElement* manifestElem);
    // Loads a standalone <Manifest> data file (e.g. physics material presets)
    // independent of any scene, so it's shared across all scenes and can be
    // edited without a rebuild.
    static bool loadManifestFile(const std::string& filename);

    static TextureManager s_TexManager;
    static ShaderManager s_ShaderManager;
    static CubemapManager s_CubemapManager;
    static MeshManager s_MeshManager;

private:
    struct ShaderPaths { std::string vert; std::string frag; };
    // Named rigid-body presets (e.g. "Wood", "Rubber") authored once in a
    // scene's <Manifest> and referenced from <RigidBody material="..."> to
    // avoid repeating the same mass/restitution numbers across entities.
    struct PhysicsMaterial { float mass = 1.0f; float restitution = 0.3f; float friction = 0.5f; float staticFriction = 0.5f; float rollingFriction = 0.05f; float linearDamping = 0.01f; float angularDamping = 0.05f; };

    glm::vec3 parseVec3(const std::string& text);
    glm::vec4 parseVec4(const std::string& text);
    void parseSpatial(TiXmlElement* elem, EntityID entity);
    void parseCamera(TiXmlElement* elem, EntityID entity);
    void parseGraphics(TiXmlElement* elem, EntityID entity);
    void parseTransform(TiXmlElement* elem, EntityID entity);
    void parseLight(TiXmlElement* elem, EntityID entity);
    void parseScript(TiXmlElement* elem, EntityID entity);
    void parseCollider(TiXmlElement* elem, EntityID entity);
    void parseRigidBody(TiXmlElement* elem, EntityID entity);
    void parseHierarchy(TiXmlElement* elem, EntityID entity);
    void parseSkybox(TiXmlElement* elem, EntityID entity);
    void resolveHierarchyReferences();
    ECXEventType getEventTypeFromHandlerName(const std::string& name);

    static std::vector<EntityID> s_Cameras;
    static std::vector<EntityID> s_Entities;
    static std::vector<EntityID> s_Lights;

    static std::unordered_map<std::string, std::string> s_HDRAliases;
    static std::unordered_map<std::string, std::string> s_MeshAliases;
    static std::unordered_map<std::string, ShaderPaths> s_ShaderAliases;
    static std::unordered_map<std::string, std::string> s_ScriptAliases;
    static std::unordered_map<std::string, TiXmlElement*> s_MaterialElements;
    static std::unordered_map<std::string, PhysicsMaterial> s_PhysicsMaterials;
    static TiXmlDocument s_ManifestDoc;
};