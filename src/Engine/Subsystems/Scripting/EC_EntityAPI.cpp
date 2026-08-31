#include "Engine/Subsystems/Scripting/EC_EntityAPI.h"
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"

namespace ScriptAPI
{
    float EntityAPI::getBlendFactor() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return 1.0f;
        if (!mgr.hasComponent<EC_DOD_Skybox>(entityID)) return 1.0f;
        return mgr.getComponent<EC_DOD_Skybox>(entityID).blendFactor;
    }

    void EntityAPI::setBlendFactor(float factor) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Skybox>(entityID)) return;
        mgr.getComponent<EC_DOD_Skybox>(entityID).blendFactor = glm::clamp(factor, 0.0f, 1.0f);
    }

    std::string EntityAPI::getName() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return "";
        if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return "";
        return mgr.getComponent<EC_DOD_EntityInfo>(entityID).name;
    }

    unsigned int EntityAPI::getUID() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return 0;
        if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return 0;
        return mgr.getComponent<EC_DOD_EntityInfo>(entityID).uid;
    }

    unsigned int EntityAPI::getID() {
        if (!EC_DOD_EntityManager::getInstance().isAlive(entityID)) return 0;
        return entityID;
    }

    bool EntityAPI::isActive() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return false;
        if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return false;
        return mgr.getComponent<EC_DOD_EntityInfo>(entityID).active;
    }

    void EntityAPI::activate() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return;
        mgr.getComponent<EC_DOD_EntityInfo>(entityID).active = true;
    }

    void EntityAPI::deactivate() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return;
        mgr.getComponent<EC_DOD_EntityInfo>(entityID).active = false;
    }

    glm::vec3 EntityAPI::getPosition() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec3(0);
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
        return mgr.getComponent<EC_DOD_Spatial>(entityID).position;
    }

    void EntityAPI::setPosition(float x, float y, float z) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        mgr.getComponent<EC_DOD_Spatial>(entityID).position = glm::vec3(x, y, z);
    }

    glm::vec3 EntityAPI::getVelocity() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec3(0);
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
        return mgr.getComponent<EC_DOD_Spatial>(entityID).velocity;
    }

    void EntityAPI::setVelocity(float x, float y, float z) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        mgr.getComponent<EC_DOD_Spatial>(entityID).velocity = glm::vec3(x, y, z);
    }

    glm::vec3 EntityAPI::getOrientation() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec3(0);
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
        return mgr.getComponent<EC_DOD_Spatial>(entityID).orientation;
    }

    void EntityAPI::setOrientation(float x, float y, float z) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        mgr.getComponent<EC_DOD_Spatial>(entityID).orientation = glm::vec3(x, y, z);
    }

    glm::vec3 EntityAPI::getAngularVelocity() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec3(0);
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
        return mgr.getComponent<EC_DOD_Spatial>(entityID).angVelocity;
    }

    void EntityAPI::setAngularVelocity(float x, float y, float z) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        mgr.getComponent<EC_DOD_Spatial>(entityID).angVelocity = glm::vec3(x, y, z);
    }

    glm::vec3 EntityAPI::getForward() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec3(0, 0, -1);
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0, 0, -1);
        return mgr.getComponent<EC_DOD_Spatial>(entityID).direction;
    }

    glm::vec3 EntityAPI::getUp() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec3(0, 1, 0);
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0, 1, 0);
        return mgr.getComponent<EC_DOD_Spatial>(entityID).up;
    }

    glm::vec3 EntityAPI::getRight() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec3(1, 0, 0);
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(1, 0, 0);
        return mgr.getComponent<EC_DOD_Spatial>(entityID).right;
    }

    void EntityAPI::moveForward(float amount) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
        spatial.position += spatial.direction * amount;
    }

    void EntityAPI::moveBack(float amount) { moveForward(-amount); }

    void EntityAPI::moveLeft(float amount) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
        spatial.position -= spatial.right * amount;
    }

    void EntityAPI::moveRight(float amount) { moveLeft(-amount); }

    void EntityAPI::moveUp(float amount) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
        spatial.position += spatial.up * amount;
    }

    void EntityAPI::moveDown(float amount) { moveUp(-amount); }

    void EntityAPI::rotateAroundAxis(float angle, float x, float y, float z) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
        auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
        spatial.orientation += glm::vec3(x, y, z) * angle;
    }

    glm::vec4 EntityAPI::getColour() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return glm::vec4(1.0f);
        if (!mgr.hasComponent<EC_DOD_GraphicsData>(entityID)) return glm::vec4(1.0f);
        return mgr.getComponent<EC_DOD_GraphicsData>(entityID).colour;
    }

    void EntityAPI::setColour(float r, float g, float b, float a) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_GraphicsData>(entityID)) return;
        mgr.getComponent<EC_DOD_GraphicsData>(entityID).colour = glm::vec4(r, g, b, a);
    }

    // Hierarchy queries
    bool EntityAPI::hasParent() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return false;
        if (!mgr.hasComponent<EC_DOD_Hierarchy>(entityID)) return false;
        return mgr.getComponent<EC_DOD_Hierarchy>(entityID).parent != INVALID_ENTITY;
    }

    unsigned int EntityAPI::getParentID() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return 0;
        if (!mgr.hasComponent<EC_DOD_Hierarchy>(entityID)) return 0;
        return mgr.getComponent<EC_DOD_Hierarchy>(entityID).parent;
    }

    unsigned int EntityAPI::getDepth() {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return 0;
        if (!mgr.hasComponent<EC_DOD_Hierarchy>(entityID)) return 0;
        return mgr.getComponent<EC_DOD_Hierarchy>(entityID).depth;
    }

    // Script variables
    void EntityAPI::setFloat(const std::string& name, float value) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return;
        mgr.getComponent<EC_DOD_ScriptData>(entityID).floatVars[name] = value;
    }

    float EntityAPI::getFloat(const std::string& name, float defaultVal) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return defaultVal;
        if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return defaultVal;
        auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
        auto it = script.floatVars.find(name);
        return (it != script.floatVars.end()) ? it->second : defaultVal;
    }

    void EntityAPI::setString(const std::string& name, const std::string& value) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return;
        if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return;
        mgr.getComponent<EC_DOD_ScriptData>(entityID).stringVars[name] = value;
    }

    std::string EntityAPI::getString(const std::string& name, const std::string& defaultVal) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.isAlive(entityID)) return defaultVal;
        if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return defaultVal;
        auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
        auto it = script.stringVars.find(name);
        return (it != script.stringVars.end()) ? it->second : defaultVal;
    }
}
