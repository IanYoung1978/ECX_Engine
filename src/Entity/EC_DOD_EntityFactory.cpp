#include "EC_DOD_EntityFactory.h"
#include "Components/EC_DOD_Components.h"
#include "Logging/ECX_Logging.h"
#include <sstream>
#include <glm/gtc/constants.hpp>
#include "Graphics/ObjModel.h"
#include "Graphics/Shader.h"
#include "Graphics/ADS_TextureSet.h"
#include "Graphics/PBR_TextureSet.h"
#include "Components/EC_ScriptComponent.h"

TextureManager EC_DOD_EntityFactory::s_TexManager;
MeshManager EC_DOD_EntityFactory::s_MeshManager;
ShaderManager EC_DOD_EntityFactory::s_ShaderManager;
std::vector<EntityID> EC_DOD_EntityFactory::s_Cameras;
std::vector<EntityID> EC_DOD_EntityFactory::s_Entities;
std::vector<EntityID> EC_DOD_EntityFactory::s_Lights;

EC_DOD_EntityFactory::EC_DOD_EntityFactory() {
    s_TexManager.init();
}

EC_DOD_EntityFactory::~EC_DOD_EntityFactory() {
}

EntityID EC_DOD_EntityFactory::constructEntity(TiXmlElement& descriptor) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EntityID entity = manager.createEntity();

    EC_DOD_EntityInfo info;
    info.active = true;

    auto att = descriptor.FirstAttribute();
    bool uidSet = false;
    while (att != nullptr) {
        if (att->NameTStr() == "uid") {
            info.uid = att->IntValue();
            uidSet = true;
        }
        if (att->NameTStr() == "name") {
            info.name = att->Value();
        }
        att = att->Next();
    }

    if (!uidSet) {
        info.uid = static_cast<uint32_t>(s_Entities.size());
    }

    manager.addComponent(entity, info);

    auto elem = descriptor.FirstChildElement();
    while (elem != nullptr) {
        if (strcmp(elem->Value(), "Spatial") == 0) {
            parseSpatial(elem, entity);
        }
        else if (strcmp(elem->Value(), "Camera") == 0) {
            parseCamera(elem, entity);
            s_Cameras.push_back(entity);
        }
        else if (strcmp(elem->Value(), "Gfx") == 0) {
            parseGraphics(elem, entity);
        }
        else if (strcmp(elem->Value(), "Transform") == 0) {
            parseTransform(elem, entity);
        }
        else if (strcmp(elem->Value(), "Light") == 0) {
            parseLight(elem, entity);
            s_Lights.push_back(entity);
        }
        else if (strcmp(elem->Value(), "ScriptComponent") == 0) {
            parseScript(elem, entity);
        }
        else if (strcmp(elem->Value(), "Collider") == 0) {
            parseCollider(elem, entity);
        }
        else {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "Unknown component type: " + std::string(elem->Value()),
                LOGGING::LogLevel::WARNING
			);
        }
        elem = elem->NextSiblingElement();
    }

    s_Entities.push_back(entity);
    return entity;
}

EntityID EC_DOD_EntityFactory::constructCamera(TiXmlElement& descriptor) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EntityID entity = manager.createEntity();

    EC_DOD_EntityInfo info;
    info.name = "Camera";
    info.active = true;
    manager.addComponent(entity, info);

    EC_DOD_Camera camera;

    auto elem = descriptor.FirstChildElement();
    while (elem) {
        if (strcmp(elem->Value(), "Spatial") == 0) {
            parseSpatial(elem, entity);
        }
        else if (strcmp(elem->Value(), "FOV") == 0) {
            camera.fov = std::stof(elem->GetText());
        }
        else if (strcmp(elem->Value(), "DrawDistance") == 0) {
            camera.farPlane = std::stof(elem->GetText());
        }

        elem = elem->NextSiblingElement();
    }
    camera.isActive = true;
    manager.addComponent(entity, camera);
    s_Cameras.push_back(entity);

    return entity;
}

void EC_DOD_EntityFactory::performPostLoadActions() {
    // First, finalize all resources (creates OpenGL objects)
    s_TexManager.finalizeTextures();
    s_MeshManager.finaliseModels();
    s_ShaderManager.finaliseShaders();

    // NOW update all graphics components with the finalized resources
    auto& manager = EC_DOD_EntityManager::getInstance();
    auto* gfxArray = manager.getComponentArray<EC_DOD_GraphicsData>();

    if (gfxArray) {
        std::unique_lock lock(gfxArray->getMutex());
        auto& gfxComponents = gfxArray->getData();

        for (size_t i = 0; i < gfxComponents.size(); i++) {
            auto& gfx = gfxComponents[i];

            // NOW retrieve the finalized resources
            if (!gfx.modelName.empty()) {
                gfx.model = s_MeshManager.getObjModel(gfx.modelName);
                if (!gfx.model) {
                    LOGGING::ECX_Logger::GetInstance()->LogMessage(
                        "Failed to get model: " + gfx.modelName,
                        LOGGING::LogLevel::SEVERE
                    );
                }
            }

            if (!gfx.vertShader.empty() && !gfx.fragShader.empty()) {
                gfx.shader = s_ShaderManager.getShader(gfx.vertShader, gfx.fragShader);
                if (!gfx.shader) {
                    LOGGING::ECX_Logger::GetInstance()->LogMessage(
                        "Failed to get shader: " + gfx.vertShader + ", " + gfx.fragShader,
                        LOGGING::LogLevel::SEVERE
                    );
                }
            }

            // Finalize texture handles
            if (gfx.hasTextures && gfx.textureSet) {
                gfx.textureSet->setTextureHandles(s_TexManager);
            }
        }
    }

    s_Entities.clear();
    s_Cameras.clear();
    s_Lights.clear();
}

const std::vector<EntityID>& EC_DOD_EntityFactory::getCameras() const {
    return s_Cameras;
}

const std::vector<EntityID>& EC_DOD_EntityFactory::getEntities() const {
    return s_Entities;
}

const std::vector<EntityID>& EC_DOD_EntityFactory::getLights() const {
    return s_Lights;
}

glm::vec3 EC_DOD_EntityFactory::parseVec3(const std::string& text) {
    glm::vec3 result(0.0f);
    std::stringstream ss(text);
    char delim;
    ss >> result.x >> delim >> result.y >> delim >> result.z;
    return result;
}

glm::vec4 EC_DOD_EntityFactory::parseVec4(const std::string& text) {
    glm::vec4 result(0.0f);
    std::stringstream ss(text);
    char delim;
    ss >> result.x >> delim >> result.y >> delim >> result.z;
    result.w = 1.0f;
    return result;
}

void EC_DOD_EntityFactory::parseSpatial(TiXmlElement* elem, EntityID entity) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EC_DOD_Spatial spatial;

    auto child = elem->FirstChildElement();
    while (child != nullptr) {
        if (strcmp(child->Value(), "Position") == 0) {
            spatial.position = parseVec3(child->GetText());
        }
        else if (strcmp(child->Value(), "Velocity") == 0) {
            spatial.velocity = parseVec3(child->GetText());
        }
        else if (strcmp(child->Value(), "Orientation") == 0) {
            glm::vec3 orient = parseVec3(child->GetText());
            spatial.orientation = glm::vec3(
                glm::radians(orient.x),
                glm::radians(orient.y),
                glm::radians(orient.z)
            );
        }
        else if (strcmp(child->Value(), "AngularVelocity") == 0) {
            glm::vec3 angVel = parseVec3(child->GetText());
            spatial.angVelocity = glm::vec3(
                glm::radians(angVel.x),
                glm::radians(angVel.y),
                glm::radians(angVel.z)
            );
        }

        child = child->NextSiblingElement();
    }

    manager.addComponent(entity, spatial);
}

void EC_DOD_EntityFactory::parseCamera(TiXmlElement* elem, EntityID entity) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EC_DOD_Camera camera;

    auto child = elem->FirstChildElement();
    while (child) {
        if (strcmp(child->Value(), "FOV") == 0) {
            camera.fov = std::stof(child->GetText());
        }
        else if (strcmp(child->Value(), "DrawDistance") == 0) {
            camera.farPlane = std::stof(child->GetText());
        }

        child = child->NextSiblingElement();
    }

    manager.addComponent(entity, camera);
}

void EC_DOD_EntityFactory::parseGraphics(TiXmlElement* elem, EntityID entity) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EC_DOD_GraphicsData gfx;

    std::string modelName;
    std::string vertShader;
    std::string fragShader;

    auto child = elem->FirstChildElement();
    while (child != nullptr) {
        if (strcmp(child->Value(), "Model") == 0) {
            modelName = child->GetText();
            s_MeshManager.loadObjModel(modelName);
        }
        else if (strcmp(child->Value(), "Shader") == 0) {
            auto vertex = child->FirstAttribute();
            if (vertex) {
                vertShader = vertex->Value();
                auto frag = vertex->Next();
                if (frag && !vertShader.empty()) {
                    fragShader = frag->Value();
                    if (!fragShader.empty()) {
                        s_ShaderManager.loadShader(vertShader, fragShader);
                    }
                }
            }
        }
        else if (strcmp(child->Value(), "ADSMaterial") == 0) {
            gfx.textureSet = std::make_shared<ADS_TextureSet>();
            gfx.hasTextures = true;

            auto child1 = child->FirstChildElement();
            while (child1 != nullptr) {
                std::string texName = child1->GetText();
                s_TexManager.loadTexture(texName);

                if (strcmp(child1->Value(), "Diffuse") == 0) {
                    gfx.textureSet->setTexture(TextureID::Diffuse, texName);
                }
                else if (strcmp(child1->Value(), "Normal") == 0) {
                    gfx.textureSet->setTexture(TextureID::Normal, texName);
                }
                else if (strcmp(child1->Value(), "Specular") == 0) {
                    gfx.textureSet->setTexture(TextureID::Specular, texName);
                }
                else if (strcmp(child1->Value(), "Height") == 0) {
                    gfx.textureSet->setTexture(TextureID::Parallax, texName);
                }
                else if (strcmp(child1->Value(), "Emissive") == 0) {
                    gfx.textureSet->setTexture(TextureID::Glow, texName);
                }

                child1 = child1->NextSiblingElement();
            }
        }
        else if (strcmp(child->Value(), "PBRMaterial") == 0) {
            gfx.textureSet = std::make_shared<PBR_TextureSet>();
            gfx.hasTextures = true;

            auto child1 = child->FirstChildElement();
            while (child1 != nullptr) {
                std::string texName = child1->GetText();
                s_TexManager.loadTexture(texName);

                if (strcmp(child1->Value(), "Albedo") == 0) {
                    gfx.textureSet->setTexture(TextureID::Albedo, texName);
                }
                else if (strcmp(child1->Value(), "Normal") == 0) {
                    gfx.textureSet->setTexture(TextureID::Normal, texName);
                }
                else if (strcmp(child1->Value(), "Smoothness") == 0) {
                    gfx.textureSet->setTexture(TextureID::Smoothness, texName);
                }
                else if (strcmp(child1->Value(), "Height") == 0) {
                    gfx.textureSet->setTexture(TextureID::Parallax, texName);
                }
                else if (strcmp(child1->Value(), "Emissive") == 0) {
                    gfx.textureSet->setTexture(TextureID::Glow, texName);
                }
                else if (strcmp(child1->Value(), "Metallic") == 0) {
                    gfx.textureSet->setTexture(TextureID::Metallic, texName);
                }
                else if (strcmp(child1->Value(), "AO") == 0) {
                    gfx.textureSet->setTexture(TextureID::AO, texName);
                }

                child1 = child1->NextSiblingElement();
            }
        }
        else if (strcmp(child->Value(), "Colour") == 0 || strcmp(child->Value(), "Color") == 0) {
            std::string color = child->GetText();
            sscanf(color.c_str(), "%f,%f,%f,%f",
                &gfx.colour.r, &gfx.colour.g, &gfx.colour.b, &gfx.colour.a);
        }
        else {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "Unknown graphics property: " + std::string(child->Value()),
                LOGGING::LogLevel::WARNING
			);
        }

        child = child->NextSiblingElement();
    }

    // Store resource names
    gfx.modelName = modelName;
    gfx.vertShader = vertShader;
    gfx.fragShader = fragShader;

    manager.addComponent(entity, gfx);
}

void EC_DOD_EntityFactory::parseTransform(TiXmlElement* elem, EntityID entity) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EC_DOD_Transform transform;

    auto child = elem->FirstChildElement();
    while (child) {
        if (strcmp(child->Value(), "Scale") == 0) {
            glm::vec3 scale = parseVec3(child->GetText());
            transform.localTransform = glm::scale(glm::mat4(1.0f), scale);
        }

        child = child->NextSiblingElement();
    }

    manager.addComponent(entity, transform);
}


void EC_DOD_EntityFactory::parseScript(TiXmlElement* elem, EntityID entity) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EC_DOD_ScriptData script;
    script.enabled = true;

    auto attr = elem->Attribute("file");
    if (attr != nullptr) {
        script.scriptFile = std::string(attr);
    }
    else {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "ScriptComponent defined without a script file!",
            LOGGING::LogLevel::WARNING
        );
    }

    manager.addComponent(entity, script);
}

void EC_DOD_EntityFactory::parseLight(TiXmlElement* elem, EntityID entity) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EC_DOD_Light light;

    auto child = elem->FirstChildElement();
    while (child != nullptr) {
        if (strcmp(child->Value(), "Type") == 0) {
            std::string type = child->GetText();
            if (type == "Directional") {
                light.type = EC_DOD_Light::Type::Directional;
            }
            else if (type == "Spotlight") {
                light.type = EC_DOD_Light::Type::Spot;
            }
            else if (type == "Point") {
                light.type = EC_DOD_Light::Type::Point;
            }
        }
        else if (strcmp(child->Value(), "Position") == 0) {
            light.position = parseVec3(child->GetText());
        }
        else if (strcmp(child->Value(), "Colour") == 0) {
            light.colour = parseVec3(child->GetText());
        }
        else if (strcmp(child->Value(), "Direction") == 0) {
            glm::vec3 dir = parseVec3(child->GetText());
            light.direction = glm::normalize(dir);
        }
        else if (strcmp(child->Value(), "Intensity") == 0) {
            light.intensity = std::stof(child->GetText());
        }
        else if (strcmp(child->Value(), "Cutoff") == 0) {
            light.cutoffAngle = glm::radians(std::stof(child->GetText()) / 2.0f);
        }
        else if (strcmp(child->Value(), "Attenuation") == 0) {
            light.attenuation = parseVec3(child->GetText());
        }
        else if (strcmp(child->Value(), "CastsShadow") == 0) {
            light.castsShadow = (strcmp(child->GetText(), "true") == 0);
        }
        else if (strcmp(child->Value(), "Dynamic") == 0) {
            light.dynamic = (strcmp(child->GetText(), "true") == 0);
        }

        child = child->NextSiblingElement();
    }

    manager.addComponent(entity, light);
}

void EC_DOD_EntityFactory::parseCollider(TiXmlElement* elem, EntityID entity) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    EC_DOD_Collider collider;

    // Parse collider type
    auto typeElem = elem->FirstChildElement("Type");
    if (typeElem && typeElem->GetText()) {
        std::string type = typeElem->GetText();
        if (type == "Sphere") collider.type = EC_DOD_Collider::Type::Sphere;
        else if (type == "AABB") collider.type = EC_DOD_Collider::Type::AABB;
        else if (type == "OBB") collider.type = EC_DOD_Collider::Type::OBB;
        else if (type == "Capsule") collider.type = EC_DOD_Collider::Type::Capsule;
        else if (type == "Cylinder") collider.type = EC_DOD_Collider::Type::Cylinder;
        else if (type == "Frustum") collider.type = EC_DOD_Collider::Type::Frustum;
        else if (type == "Plane") collider.type = EC_DOD_Collider::Type::Plane;
    }

    // Parse radius (for Sphere, Capsule, Cylinder)
    auto radiusElem = elem->FirstChildElement("Radius");
    if (radiusElem && radiusElem->GetText()) {
        collider.radius = static_cast<float>(atof(radiusElem->GetText()));
    }

    // Parse extents (for AABB, OBB)
    auto extentsElem = elem->FirstChildElement("Extents");
    if (extentsElem && extentsElem->GetText()) {
        std::string extents = extentsElem->GetText();
        sscanf(extents.c_str(), "%f,%f,%f",
            &collider.extents.x, &collider.extents.y, &collider.extents.z);
    }

    // Parse height (for Capsule, Cylinder)
    auto heightElem = elem->FirstChildElement("Height");
    if (heightElem && heightElem->GetText()) {
        collider.height = static_cast<float>(atof(heightElem->GetText()));
    }

    // Parse center offset
    auto centerElem = elem->FirstChildElement("Center");
    if (centerElem && centerElem->GetText()) {
        std::string center = centerElem->GetText();
        sscanf(center.c_str(), "%f,%f,%f",
            &collider.center.x, &collider.center.y, &collider.center.z);
    }

    // Parse collision layer
    auto layerElem = elem->FirstChildElement("Layer");
    if (layerElem && layerElem->GetText()) {
        collider.collisionLayer = static_cast<uint32_t>(atoi(layerElem->GetText()));
    }

    // Parse collision mask
    auto maskElem = elem->FirstChildElement("Mask");
    if (maskElem && maskElem->GetText()) {
        collider.collisionMask = static_cast<uint32_t>(strtoul(maskElem->GetText(), nullptr, 0));
    }

    manager.addComponent(entity, collider);
}