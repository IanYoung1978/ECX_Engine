#pragma once
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include <glm/glm.hpp>
#include <string>

namespace ScriptAPI
{
    struct EntityAPI
    {
        EntityID entityID;

        EntityAPI(EntityID id) : entityID(id) {}

        std::string getName() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return "";
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return "";
            return mgr.getComponent<EC_DOD_EntityInfo>(entityID).name;
        }

        unsigned int getUID() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return 0;
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return 0;
            return mgr.getComponent<EC_DOD_EntityInfo>(entityID).uid;
        }

        unsigned int getID() {
            if (!EC_DOD_EntityManager::getInstance().isAlive(entityID)) return 0;
            return entityID;
        }

        bool isActive() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return false;
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return false;
            return mgr.getComponent<EC_DOD_EntityInfo>(entityID).active;
        }

        void activate() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return;
            mgr.getComponent<EC_DOD_EntityInfo>(entityID).active = true;
        }

        void deactivate() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_EntityInfo>(entityID)) return;
            mgr.getComponent<EC_DOD_EntityInfo>(entityID).active = false;
        }

        glm::vec3 getPosition() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec3(0);
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).position;
        }

        void setPosition(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            mgr.getComponent<EC_DOD_Spatial>(entityID).position = glm::vec3(x, y, z);
        }

        glm::vec3 getVelocity() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec3(0);
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).velocity;
        }

        void setVelocity(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            mgr.getComponent<EC_DOD_Spatial>(entityID).velocity = glm::vec3(x, y, z);
        }

        glm::vec3 getOrientation() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec3(0);
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).orientation;
        }

        void setOrientation(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            mgr.getComponent<EC_DOD_Spatial>(entityID).orientation = glm::vec3(x, y, z);
        }

        glm::vec3 getAngularVelocity() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec3(0);
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).angVelocity;
        }

        void setAngularVelocity(float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            mgr.getComponent<EC_DOD_Spatial>(entityID).angVelocity = glm::vec3(x, y, z);
        }

        glm::vec3 getForward() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec3(0, 0, -1);
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0, 0, -1);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).direction;
        }

        glm::vec3 getUp() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec3(0, 1, 0);
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(0, 1, 0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).up;
        }

        glm::vec3 getRight() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec3(1, 0, 0);
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return glm::vec3(1, 0, 0);
            return mgr.getComponent<EC_DOD_Spatial>(entityID).right;
        }

        void moveForward(float amount) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.position += spatial.direction * amount;
        }

        void moveBack(float amount) { moveForward(-amount); }

        void moveLeft(float amount) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.position -= spatial.right * amount;
        }

        void moveRight(float amount) { moveLeft(-amount); }

        void moveUp(float amount) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.position += spatial.up * amount;
        }

        void moveDown(float amount) { moveUp(-amount); }

        void rotateAroundAxis(float angle, float x, float y, float z) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_Spatial>(entityID)) return;
            auto& spatial = mgr.getComponent<EC_DOD_Spatial>(entityID);
            spatial.orientation += glm::vec3(x, y, z) * angle;
        }

        glm::vec4 getColour() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return glm::vec4(1.0f);
            if (!mgr.hasComponent<EC_DOD_GraphicsData>(entityID)) return glm::vec4(1.0f);
            return mgr.getComponent<EC_DOD_GraphicsData>(entityID).colour;
        }

        void setColour(float r, float g, float b, float a = 1.0f) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_GraphicsData>(entityID)) return;
            mgr.getComponent<EC_DOD_GraphicsData>(entityID).colour = glm::vec4(r, g, b, a);
        }

        // Hierarchy queries
        bool hasParent() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return false;
            if (!mgr.hasComponent<EC_DOD_Hierarchy>(entityID)) return false;
            return mgr.getComponent<EC_DOD_Hierarchy>(entityID).parent != INVALID_ENTITY;
        }

        unsigned int getParentID() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return 0;
            if (!mgr.hasComponent<EC_DOD_Hierarchy>(entityID)) return 0;
            return mgr.getComponent<EC_DOD_Hierarchy>(entityID).parent;
        }

        unsigned int getDepth() {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return 0;
            if (!mgr.hasComponent<EC_DOD_Hierarchy>(entityID)) return 0;
            return mgr.getComponent<EC_DOD_Hierarchy>(entityID).depth;
        }

        // Script variables
        void setFloat(const std::string& name, float value) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return;
            mgr.getComponent<EC_DOD_ScriptData>(entityID).floatVars[name] = value;
        }

        float getFloat(const std::string& name, float defaultVal = 0.0f) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return defaultVal;
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return defaultVal;
            auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
            auto it = script.floatVars.find(name);
            return (it != script.floatVars.end()) ? it->second : defaultVal;
        }

        void setString(const std::string& name, const std::string& value) {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return;
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return;
            mgr.getComponent<EC_DOD_ScriptData>(entityID).stringVars[name] = value;
        }

        std::string getString(const std::string& name, const std::string& defaultVal = "") {
            auto& mgr = EC_DOD_EntityManager::getInstance();
            if (!mgr.isAlive(entityID)) return defaultVal;
            if (!mgr.hasComponent<EC_DOD_ScriptData>(entityID)) return defaultVal;
            auto& script = mgr.getComponent<EC_DOD_ScriptData>(entityID);
            auto it = script.stringVars.find(name);
            return (it != script.stringVars.end()) ? it->second : defaultVal;
        }
    };
}