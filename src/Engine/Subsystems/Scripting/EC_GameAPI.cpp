#include "Engine/Subsystems/Scripting/EC_GameAPI.h"
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Engine/Subsystems/Scripting/EC_EntityAPI.h"
#include "UI/EC_UI_Components.h"
#include "Logging/ECX_Logging.h"
#include "Game.h"
#include "Messaging/ECXMessenger.h"
#include <algorithm>

namespace ScriptAPI
{
    EntityAPI GameAPI::getEntityByName(const std::string& name) {
        if (!game) return EntityAPI(INVALID_ENTITY);
        return EntityAPI(game->getEntityByName(name));
    }

    unsigned int GameAPI::getEntityIDByUID(unsigned int uid) {
        if (!game) return INVALID_ENTITY;
        return game->getEntityByUID(static_cast<uint32_t>(uid));
    }

    void GameAPI::shutdown() {
        if (game) game->shutDown();
    }

    void GameAPI::pauseGame() {
        if (game) game->pauseGame();
    }

    void GameAPI::resumeGame() {
        if (game) game->resumeGame();
    }

    int GameAPI::getKeyState(const std::string& key) {
        if (!game) return 0;
        SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());
        KeyState state = game->getKeyState(scancode);
        return static_cast<int>(state);
    }

    void GameAPI::setParent(unsigned int childID, unsigned int parentID) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID child = static_cast<EntityID>(childID);
        EntityID parent = static_cast<EntityID>(parentID);

        if (!mgr.isAlive(child)) return;
        if (!mgr.isAlive(parent)) return;

        if (!mgr.hasComponent<EC_DOD_Hierarchy>(child))
            mgr.addComponent(child, EC_DOD_Hierarchy{});
        if (!mgr.hasComponent<EC_DOD_Hierarchy>(parent))
            mgr.addComponent(parent, EC_DOD_Hierarchy{});

        auto& childHierarchy = mgr.getComponent<EC_DOD_Hierarchy>(child);
        EntityID oldParent = childHierarchy.parent;
        if (oldParent == parent) return;

        // Remove from old parent
        if (oldParent != INVALID_ENTITY && mgr.hasComponent<EC_DOD_Hierarchy>(oldParent)) {
            auto& oldParentHierarchy = mgr.getComponent<EC_DOD_Hierarchy>(oldParent);
            auto& children = oldParentHierarchy.children;
            children.erase(std::remove(children.begin(), children.end(), child), children.end());
        }

        // Set new parent
        childHierarchy.parent = parent;
        mgr.getComponent<EC_DOD_Hierarchy>(parent).children.push_back(child);

        // Update depths
        updateDepth(child, mgr.getComponent<EC_DOD_Hierarchy>(parent).depth + 1);

        // Mark transform dirty
        if (mgr.hasComponent<EC_DOD_Transform>(child))
            mgr.getComponent<EC_DOD_Transform>(child).dirty = true;
    }

    void GameAPI::clearParent(unsigned int childID) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID child = static_cast<EntityID>(childID);

        if (!mgr.isAlive(child)) return;
        if (!mgr.hasComponent<EC_DOD_Hierarchy>(child)) return;

        auto& hierarchy = mgr.getComponent<EC_DOD_Hierarchy>(child);
        EntityID oldParent = hierarchy.parent;
        if (oldParent == INVALID_ENTITY) return;

        // Remove from old parent's children list
        if (mgr.hasComponent<EC_DOD_Hierarchy>(oldParent)) {
            auto& children = mgr.getComponent<EC_DOD_Hierarchy>(oldParent).children;
            children.erase(std::remove(children.begin(), children.end(), child), children.end());
        }

        // Clear parent and reset depth
        hierarchy.parent = INVALID_ENTITY;
        updateDepth(child, 0);

        // Mark transform dirty
        if (mgr.hasComponent<EC_DOD_Transform>(child))
            mgr.getComponent<EC_DOD_Transform>(child).dirty = true;
    }

    void GameAPI::setExposure(float exposure) {
        if (!messenger) return;
        ECXCommand cmd;
        cmd.type = ECXCommandType::GraphicsChangeHDRExposure;
        cmd.args[0] = exposure;
        messenger->publish(cmd);
    }

    void GameAPI::setAmbientScale(float scale) {
        if (!messenger) return;
        ECXCommand cmd;
        cmd.type = ECXCommandType::GraphicsChangeAmbientScale;
        cmd.args[0] = scale;
        messenger->publish(cmd);
    }

    void GameAPI::toggleDebug() {
        if (!messenger) return;
        ECXCommand cmd;
        cmd.type = ECXCommandType::GraphicsToggleDebug;
        messenger->publish(cmd);
    }

    void GameAPI::setResolution(int width, int height) {
        if (!game) return;
        game->setResolution(width, height);
    }

    void GameAPI::toggleFullscreen() {
        if (!game) return;
        game->toggleFullscreen();
    }

    void GameAPI::maximizeWindow() {
        if (!game) return;
        game->maximizeWindow();
    }

    void GameAPI::minimizeWindow() {
        if (!game) return;
        game->minimizeWindow();
    }

    void GameAPI::loadScene(const std::string& alias) {
        if (game) game->loadScene(alias);
    }

    void GameAPI::unloadScene(const std::string& alias) {
        if (game) game->unloadScene(alias);
    }

    void GameAPI::activateScene(const std::string& alias) {
        if (game) game->activateScene(alias);
    }

    void GameAPI::setUIText(unsigned int entityID, const std::string& text) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = static_cast<EntityID>(entityID);
        if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Text>(entity)) return;
        mgr.getComponent<EC_UI_Text>(entity).text = text;
    }

    void GameAPI::setUITextColour(unsigned int entityID, float r, float g, float b, float a) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = static_cast<EntityID>(entityID);
        if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Text>(entity)) return;
        mgr.getComponent<EC_UI_Text>(entity).colour = glm::vec4(r, g, b, a);
    }

    void GameAPI::setUIPanelColour(unsigned int entityID, float r, float g, float b, float a) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = static_cast<EntityID>(entityID);
        if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Panel>(entity)) return;
        mgr.getComponent<EC_UI_Panel>(entity).colour = glm::vec4(r, g, b, a);
    }

    void GameAPI::setUIVisible(unsigned int entityID, bool visible) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = static_cast<EntityID>(entityID);
        if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
        mgr.getComponent<EC_UI_Element>(entity).visible = visible;
    }

    void GameAPI::setUIPosition(unsigned int entityID, float x, float y) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = static_cast<EntityID>(entityID);
        if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
        mgr.getComponent<EC_UI_Element>(entity).position = glm::vec2(x, y);
    }

    void GameAPI::setUISize(unsigned int entityID, float w, float h) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = static_cast<EntityID>(entityID);
        if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
        mgr.getComponent<EC_UI_Element>(entity).size = glm::vec2(w, h);
    }

    void GameAPI::setUILayer(unsigned int entityID, int layer) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = static_cast<EntityID>(entityID);
        if (!mgr.isAlive(entity) || !mgr.hasComponent<EC_UI_Element>(entity)) return;
        mgr.getComponent<EC_UI_Element>(entity).layer = layer;
    }

    unsigned int GameAPI::createUIElement(float x, float y, float w, float h, int layer) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        EntityID entity = mgr.createEntity();
        EC_UI_Element element;
        element.position = glm::vec2(x, y);
        element.size = glm::vec2(w, h);
        element.layer = layer;
        mgr.addComponent(entity, element);
        return entity;
    }

    float GameAPI::getFPS() {
        if (!game) return 0.0f;
        return game->getFPS();
    }

    float GameAPI::getMSPF() {
        if (!game) return 0.0f;
        return game->getMSPF();
    }

    int GameAPI::getRecentLogCount() {
        return static_cast<int>(LOGGING::ECX_Logger::GetInstance()->GetRecentPlainLogs(200).size());
    }

    std::string GameAPI::getRecentLog(int index) {
        auto logs = LOGGING::ECX_Logger::GetInstance()->GetRecentPlainLogs(200);
        if (index < 0 || static_cast<size_t>(index) >= logs.size()) return "";
        return logs[index];
    }

    void GameAPI::setMouseCaptured(bool captured) {
        if (game) game->setMouseCaptured(captured);
    }

    void GameAPI::log(const std::string& message) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(message, LOGGING::LogLevel::INFORMATION);
    }

    int GameAPI::rayQuery(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance, bool firstHitOnly) {
        if (!game) { m_LastRayHits.clear(); return 0; }
        m_LastRayHits = game->queryRay(glm::vec3(ox, oy, oz), glm::vec3(dx, dy, dz), maxDistance, firstHitOnly);
        return static_cast<int>(m_LastRayHits.size());
    }

    EntityAPI GameAPI::getRayHitEntity(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return EntityAPI(INVALID_ENTITY);
        return EntityAPI(m_LastRayHits[index].entity);
    }

    glm::vec3 GameAPI::getRayHitPosition(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return glm::vec3(0.0f);
        return m_LastRayHits[index].position;
    }

    glm::vec3 GameAPI::getRayHitNormal(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return glm::vec3(0.0f);
        return m_LastRayHits[index].normal;
    }

    float GameAPI::getRayHitDistance(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastRayHits.size()) return 0.0f;
        return m_LastRayHits[index].distance;
    }

    int GameAPI::coneQuery(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance, bool castsShadowOnly, bool checkOcclusion) {
        if (!game) { m_LastConeHits.clear(); return 0; }
        m_LastConeHits = game->queryCone(glm::vec3(ax, ay, az), glm::vec3(dx, dy, dz), halfAngleDegrees, maxDistance, castsShadowOnly, checkOcclusion);
        return static_cast<int>(m_LastConeHits.size());
    }

    EntityAPI GameAPI::getConeHitEntity(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastConeHits.size()) return EntityAPI(INVALID_ENTITY);
        return EntityAPI(m_LastConeHits[index].entity);
    }

    glm::vec3 GameAPI::getConeHitPosition(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastConeHits.size()) return glm::vec3(0.0f);
        return m_LastConeHits[index].position;
    }

    float GameAPI::getConeHitDistance(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastConeHits.size()) return 0.0f;
        return m_LastConeHits[index].distance;
    }

    void GameAPI::showDebugRay(float ox, float oy, float oz, float dx, float dy, float dz, float maxDistance) {
        if (!game) return;
        game->showDebugRay(glm::vec3(ox, oy, oz), glm::vec3(dx, dy, dz), maxDistance);
    }

    void GameAPI::showDebugCone(float ax, float ay, float az, float dx, float dy, float dz, float halfAngleDegrees, float maxDistance) {
        if (!game) return;
        game->showDebugCone(glm::vec3(ax, ay, az), glm::vec3(dx, dy, dz), halfAngleDegrees, maxDistance);
    }

    void GameAPI::regenerateTerrain() {
        if (!game) return;
        game->regenerateTerrain();
    }

    int GameAPI::capsuleQuery(float ax, float ay, float az, float bx, float by, float bz, float radius,
        bool firstHitOnly, unsigned int excludeEntityId) {
        if (!game) { m_LastCapsuleHits.clear(); return 0; }
        m_LastCapsuleHits = game->queryCapsule(glm::vec3(ax, ay, az), glm::vec3(bx, by, bz), radius,
            firstHitOnly, 0xFFFFFFFFu, static_cast<EntityID>(excludeEntityId));
        return static_cast<int>(m_LastCapsuleHits.size());
    }

    EntityAPI GameAPI::getCapsuleHitEntity(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastCapsuleHits.size()) return EntityAPI(INVALID_ENTITY);
        return EntityAPI(m_LastCapsuleHits[index].entity);
    }

    glm::vec3 GameAPI::getCapsuleHitPosition(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastCapsuleHits.size()) return glm::vec3(0.0f);
        return m_LastCapsuleHits[index].position;
    }

    glm::vec3 GameAPI::getCapsuleHitNormal(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastCapsuleHits.size()) return glm::vec3(0.0f);
        return m_LastCapsuleHits[index].normal;
    }

    float GameAPI::getCapsuleHitDistance(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_LastCapsuleHits.size()) return 0.0f;
        return m_LastCapsuleHits[index].distance;
    }

    void GameAPI::playSound(const std::string& path, float volume, const std::string& category) {
        if (!game) return;
        game->playSound(path, volume, category);
    }

    void GameAPI::playMusic(const std::string& path, float volume, bool loop) {
        if (!game) return;
        game->playMusic(path, volume, loop);
    }

    void GameAPI::stopMusic() {
        if (!game) return;
        game->stopMusic();
    }

    void GameAPI::setCategoryVolume(const std::string& category, float volume) {
        if (!game) return;
        game->setCategoryVolume(category, volume);
    }

    void GameAPI::updateDepth(EntityID entity, uint32_t depth) {
        auto& mgr = EC_DOD_EntityManager::getInstance();
        if (!mgr.hasComponent<EC_DOD_Hierarchy>(entity)) return;
        auto& hierarchy = mgr.getComponent<EC_DOD_Hierarchy>(entity);
        hierarchy.depth = depth;
        for (EntityID child : hierarchy.children)
            updateDepth(child, depth + 1);
    }
}
