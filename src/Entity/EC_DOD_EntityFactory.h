#pragma once
#include "xml/tinyxml.h"
#include "EC_DOD_EntityManager.h"
#include "Graphics/TextureManager.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/MeshManager.h"
#include <vector>
#include <string>
#include <cstdint>
#include "EC_DOD_Types.h"

class EC_DOD_EntityFactory {
public:
    EC_DOD_EntityFactory();
    virtual ~EC_DOD_EntityFactory();

    EntityID constructEntity(TiXmlElement& descriptor);
    EntityID constructCamera(TiXmlElement& descriptor);

    void performPostLoadActions();

    const std::vector<EntityID>& getCameras() const;
    const std::vector<EntityID>& getEntities() const;
    const std::vector<EntityID>& getLights() const;

    static TextureManager s_TexManager;
    static ShaderManager s_ShaderManager;
    static MeshManager s_MeshManager;

private:
    glm::vec3 parseVec3(const std::string& text);
    glm::vec4 parseVec4(const std::string& text);

    void parseSpatial(TiXmlElement* elem, EntityID entity);
    void parseCamera(TiXmlElement* elem, EntityID entity);
    void parseGraphics(TiXmlElement* elem, EntityID entity);
    void parseTransform(TiXmlElement* elem, EntityID entity);
    void parseLight(TiXmlElement* elem, EntityID entity);
    void parseScript(TiXmlElement* elem, EntityID entity);
    void parseCollider(TiXmlElement* elem, EntityID entity);
    static std::vector<EntityID> s_Cameras;
    static std::vector<EntityID> s_Entities;
    static std::vector<EntityID> s_Lights;
};