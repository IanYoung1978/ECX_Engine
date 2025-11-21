#pragma once
#include <lua.hpp>
#include <luabridge3/LuaBridge/LuaBridge.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <any>
#include "Entity/GameEntity.h"
#include "Messaging/ECXEvent.h"
#include "Messaging/KeyEvent.h"
#include "Messaging/MouseEvent.h"
#include "Components/EC_ScriptComponent.h"
#include "Engine/Subsystems/EC_System.h"
#include "Messaging/IEventListener.h"
#include "Game.h"
#include "Entity/EntityManager.h"
#include "Logging/ECX_Logging.h"
#include <fstream>
#include <iostream>

struct ScriptComponent {
    std::string scriptFile;
    std::unordered_map<std::string, float> floatVars;
    std::unordered_map<std::string, std::string> stringVars;
    bool enabled = true;  // Can disable script without removing component
};

class EC_LuaScriptSystem : public EC_System, public IEventListener {
public:
    EC_LuaScriptSystem() : m_luaState(nullptr), m_game(nullptr) {}

    ~EC_LuaScriptSystem() {
        if (m_luaState) lua_close(m_luaState);
    }

    void init(ECXMessenger& messenger, EC_Game& game) override {
        // Subscribe to ALL events
        std::vector<ECXEventType> allTypes{
            ECXEventType::EntityCreate,
            ECXEventType::EntityKill,
            ECXEventType::EntityDestroy,
            ECXEventType::EntityStopRotation,
            ECXEventType::EntityStopMotion,
            ECXEventType::EntityChangePosition,
            ECXEventType::EntityChangeOrientation,
            ECXEventType::EntityChangeAngularVelocity,
            ECXEventType::EntityChangeVelocity,
            ECXEventType::CollisionBeginEvent,
            ECXEventType::CollisionEndEvent,
            ECXEventType::key_up,
            ECXEventType::key_down,
            ECXEventType::key_held,
            ECXEventType::mouse_up,
            ECXEventType::mouse_down,
            ECXEventType::mouse_held,
            ECXEventType::mouse_move,
            ECXEventType::world_loaded,
            ECXEventType::entity_loaded,
            ECXEventType::config_loaded,
            ECXEventType::system_update
        };
        messenger.Subscribe(*this, allTypes);

        m_game = &game;

        // Create shared Lua state
        m_luaState = luaL_newstate();
        luaL_openlibs(m_luaState);
        registerAPI();
        LOGGING::ECX_Logger::GetInstance()->LogMessage("Scripting system initialised", LOGGING::LogLevel::INFORMATION);
    }

    void update(const float& deltaTimeS, EC_Game& game) override {
        auto entities = EntityManager::getInstance()
            .getEntitiesWithComponent(std::type_index(typeid(EC_ScriptComponent)));
        for (auto& entity : entities) {
            if (!entity->isActive()) continue;

            auto script = entity->getComponent<EC_ScriptComponent>();
            if (!script->isEnabled()) continue;

            callLuaFunction(script->getScriptFile(), "update", entity, deltaTimeS);
        }
    }

    void receive(ECXEvent& event) override {
        auto entities = EntityManager::getInstance()
            .getEntitiesWithComponent(std::type_index(typeid(EC_ScriptComponent)));

        for (auto& entity : entities) {
            if (!entity->isActive()) continue;

            auto script = entity->getComponent<EC_ScriptComponent>();
            if (!script->isEnabled()) continue;

            // Route event to appropriate Lua function
            const char* funcName = getEventFunctionName(event.type);
            if (funcName) {
                callLuaEvent(script->getScriptFile(), funcName, entity, event);
            }
        }
    }

private:
    lua_State* m_luaState;
    EC_Game* m_game;
    std::unordered_map<std::string, bool> m_loadedScripts;

    // Map event types to Lua function names
    const char* getEventFunctionName(ECXEventType type) {
        switch (type) {
            // Entity lifecycle events
        case ECXEventType::EntityCreate:    return "onEntityCreate";
        case ECXEventType::EntityKill:      return "onEntityKill";
        case ECXEventType::EntityDestroy:   return "onEntityDestroy";
        case ECXEventType::entity_loaded:   return "onEntityLoaded";

            // Entity state changes
        case ECXEventType::EntityStopRotation:           return "onStopRotation";
        case ECXEventType::EntityStopMotion:             return "onStopMotion";
        case ECXEventType::EntityChangePosition:         return "onPositionChanged";
        case ECXEventType::EntityChangeOrientation:      return "onOrientationChanged";
        case ECXEventType::EntityChangeAngularVelocity:  return "onAngularVelocityChanged";
        case ECXEventType::EntityChangeVelocity:         return "onVelocityChanged";

            // Collision events
        case ECXEventType::CollisionBeginEvent: return "onCollisionBegin";
        case ECXEventType::CollisionEndEvent:   return "onCollisionEnd";

            // Input events
        case ECXEventType::key_down:    return "onKeyDown";
        case ECXEventType::key_up:      return "onKeyUp";
        case ECXEventType::key_held:    return "onKeyHeld";
        case ECXEventType::mouse_down:  return "onMouseDown";
        case ECXEventType::mouse_up:    return "onMouseUp";
        case ECXEventType::mouse_held:  return "onMouseHeld";
        case ECXEventType::mouse_move:  return "onMouseMove";

            // System events
        case ECXEventType::world_loaded:    return "onWorldLoaded";
        case ECXEventType::config_loaded:   return "onConfigLoaded";
        case ECXEventType::system_update:   return "onSystemUpdate";

        default: return nullptr;
        }
    }

    bool loadScript(const std::string& filename) {
        if (m_loadedScripts[filename]) return true;

        LOGGING::ECX_Logger::GetInstance()->LogMessage("Attempting to load script: " + filename, LOGGING::LogLevel::INFORMATION);

        int result = luaL_dofile(m_luaState, filename.c_str());
        if (result != LUA_OK) {
            const char* error = lua_tostring(m_luaState, -1);
            std::string errorMsg = error ? error : "Unknown error";
            LOGGING::ECX_Logger::GetInstance()->LogMessage("Lua error loading " + filename + ": " + errorMsg + " (error code: " + std::to_string(result) + ")", LOGGING::LogLevel::SEVERE);
            lua_pop(m_luaState, 1);
            return false;
        }

        m_loadedScripts[filename] = true;
        LOGGING::ECX_Logger::GetInstance()->LogMessage("Script loaded successfully: " + filename, LOGGING::LogLevel::INFORMATION);
        return true;
    }

    void callLuaFunction(const std::string& scriptFile, const char* funcName,
        std::shared_ptr<GameEntity> entity, float deltaTime) {
        if (!loadScript(scriptFile)) return;

        try {
            luabridge::LuaRef func = luabridge::getGlobal(m_luaState, funcName);
            if (func.isFunction()) {
                EntityAPI entityAPI(entity);
                auto result = func(entityAPI, deltaTime);

                if (!result) {
                    std::cerr << "Error in " << funcName << ": "
                        << result.errorMessage() << std::endl;
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << "Exception calling " << funcName << ": " << e.what() << std::endl;
        }
    }

    void callLuaEvent(const std::string& scriptFile, const char* funcName,
        std::shared_ptr<GameEntity> entity, ECXEvent& event) {
        if (!loadScript(scriptFile)) return;

        try {
            luabridge::LuaRef func = luabridge::getGlobal(m_luaState, funcName);
            if (func.isFunction()) {
                EntityAPI entityAPI(entity);
                EventAPI eventAPI(event, m_game);
                auto result = func(entityAPI, eventAPI);

                if (!result) {
                    std::cerr << "Error in " << funcName << ": "
                        << result.errorMessage() << std::endl;
                }
            }
        }
        catch (std::exception& e) {
            std::cerr << "Exception calling " << funcName << ": " << e.what() << std::endl;
        }
    }

    // Entity API for Lua scripts
    struct EntityAPI {
        std::shared_ptr<GameEntity> entity;

        EntityAPI(std::shared_ptr<GameEntity> e) : entity(e) {}

        std::string getName() { return entity->getName(); }
        unsigned int getUID() { return entity->getUID(); }
        bool isActive() { return entity->isActive(); }
        void activate() { entity->activate(); }
        void deactivate() { entity->deactivate(); }

        // Spatial component access
        glm::vec3 getPosition() {
            auto spatial = entity->getComponent<Spatial>();
            return spatial ? spatial->getPosition() : glm::vec3(0);
        }

        void setPosition(float x, float y, float z) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) spatial->setPosition(glm::vec3(x, y, z));
        }

        glm::vec3 getVelocity() {
            auto spatial = entity->getComponent<Spatial>();
            return spatial ? spatial->getVelocity() : glm::vec3(0);
        }

        void setVelocity(float x, float y, float z) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) spatial->setVelocity(glm::vec3(x, y, z));
        }

        glm::vec3 getOrientation() {
            auto spatial = entity->getComponent<Spatial>();
            return spatial ? spatial->getOrientation() : glm::vec3(0);
        }

        void setOrientation(float x, float y, float z) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) spatial->setOrientation(glm::vec3(x, y, z));
			printf("Set orientation to (%f, %f, %f)\n", x, y, z);
        }

        glm::vec3 getAngularVelocity() {
            auto spatial = entity->getComponent<Spatial>();
            return spatial ? spatial->getAngVelocity() : glm::vec3(0);
        }

        void setAngularVelocity(float x, float y, float z) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) spatial->setAngVelocity(glm::vec3(x, y, z));
        }

        glm::vec3 getForward() {
            auto spatial = entity->getComponent<Spatial>();
            return spatial ? spatial->getForward() : glm::vec3(0, 0, -1);
        }

        glm::vec3 getUp() {
            auto spatial = entity->getComponent<Spatial>();
            return spatial ? spatial->getUp() : glm::vec3(0, 1, 0);
        }

        glm::vec3 getRight() {
            auto spatial = entity->getComponent<Spatial>();
            return spatial ? spatial->getRight() : glm::vec3(1, 0, 0);
        }

        // Convenience movement functions
        void moveForward(float amount) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) {
                auto pos = spatial->getPosition();
                auto dir = spatial->getForward();
                spatial->setPosition(pos + dir * amount);
            }
        }

        void moveBack(float amount) { moveForward(-amount); }

        void moveLeft(float amount) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) {
                auto pos = spatial->getPosition();
                auto right = spatial->getRight();
                spatial->setPosition(pos - right * amount);
            }
        }

        void moveRight(float amount) { moveLeft(-amount); }

        void moveUp(float amount) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) {
                auto pos = spatial->getPosition();
                spatial->setPosition(pos + glm::vec3(0, amount, 0));
            }
        }

        void moveDown(float amount) { moveUp(-amount); }

        void rotateAroundAxis(float angle, float x, float y, float z) {
            auto spatial = entity->getComponent<Spatial>();
            if (spatial) {
                // Implement rotation around arbitrary axis
                glm::vec3 axis(x, y, z);
                auto orientation = spatial->getOrientation();
                // Add rotation logic here based on your needs
                spatial->setOrientation(orientation + axis * angle);
            }
        }

        // Script variables (stored in ScriptComponent)
        void setFloat(const std::string& name, float value) {
            auto script = entity->getComponent<EC_ScriptComponent>();
            if (script) script->floatVars[name] = value;
        }

        float getFloat(const std::string& name, float defaultVal = 0.0f) {
            auto script = entity->getComponent<EC_ScriptComponent>();
            if (!script) return defaultVal;
            auto it = script->floatVars.find(name);
            return (it != script->floatVars.end()) ? it->second : defaultVal;
        }

        void setString(const std::string& name, const std::string& value) {
            auto script = entity->getComponent<EC_ScriptComponent>();
            if (script) script->stringVars[name] = value;
        }

        std::string getString(const std::string& name, const std::string& defaultVal = "") {
            auto script = entity->getComponent<EC_ScriptComponent>();
            if (!script) return defaultVal;
            auto it = script->stringVars.find(name);
            return (it != script->stringVars.end()) ? it->second : defaultVal;
        }
    };

    // Event API for Lua scripts
    struct EventAPI {
        ECXEvent& event;
        EC_Game* game;

        EventAPI(ECXEvent& e, EC_Game* g) : event(e), game(g) {}

        // Input events
        std::string getKey() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    // Cast from std::any in fixed-size array
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.getKeyString();
                }
                catch (const std::bad_any_cast&) {
                    return "";
                }
            }
            return "";
        }

        bool isPressed() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.isPressed();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool isHeld() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.isHeld();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool isReleased() {
            if (event.type == ECXEventType::key_down ||
                event.type == ECXEventType::key_up ||
                event.type == ECXEventType::key_held) {
                try {
                    KeyEvent keyEvent = std::any_cast<KeyEvent>(event.args[0]);
                    return keyEvent.isReleased();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }
        float getMouseMotionX() {
            if (event.type == ECXEventType::mouse_move ||
                event.type == ECXEventType::mouse_down ||
                event.type == ECXEventType::mouse_up ||
                event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return static_cast<float>(mouseEvent.getXMotion());
                }
                catch (const std::bad_any_cast&) {
                    return 0.0f;
                }
            }
            return 0.0f;
        }

        float getMouseMotionY() {
            if (event.type == ECXEventType::mouse_move ||
                event.type == ECXEventType::mouse_down ||
                event.type == ECXEventType::mouse_up ||
                event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return static_cast<float>(mouseEvent.getYMotion());
                }
                catch (const std::bad_any_cast&) {
                    return 0.0f;
                }
            }
            return 0.0f;
        }

        int getMouseButton() {
            if (event.type == ECXEventType::mouse_down ||
                event.type == ECXEventType::mouse_up ||
                event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return static_cast<int>(mouseEvent.getMouseKey());
                }
                catch (const std::bad_any_cast&) {
                    return 0;
                }
            }
            return 0;
        }

        bool mouseButtonPressed() {
            if (event.type == ECXEventType::mouse_down) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return mouseEvent.buttonPressed();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool mouseButtonHeld() {
            if (event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return mouseEvent.buttonHeld();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        bool mouseButtonReleased() {
            if (event.type == ECXEventType::mouse_up) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    return mouseEvent.buttonReleased();
                }
                catch (const std::bad_any_cast&) {
                    return false;
                }
            }
            return false;
        }

        void getMousePosition(int& x, int& y) {
            if (event.type == ECXEventType::mouse_move ||
                event.type == ECXEventType::mouse_down ||
                event.type == ECXEventType::mouse_up ||
                event.type == ECXEventType::mouse_held) {
                try {
                    MouseEvent mouseEvent = std::any_cast<MouseEvent>(event.args[0]);
                    mouseEvent.getPosition(x, y);
                }
                catch (const std::bad_any_cast&) {
                    x = 0;
                    y = 0;
                }
            }
        }

        // Collision events
        unsigned int getOtherEntityUID() {
            if (event.type == ECXEventType::CollisionBeginEvent ||
                event.type == ECXEventType::CollisionEndEvent) {
                try {
                    // Adjust this based on your actual collision event structure
                    return std::any_cast<unsigned int>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return 0;
                }
            }
            return 0;
        }

        // Entity change events - get the new value
        glm::vec3 getNewPosition() {
            if (event.type == ECXEventType::EntityChangePosition) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }

        glm::vec3 getNewOrientation() {
            if (event.type == ECXEventType::EntityChangeOrientation) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }

        glm::vec3 getNewVelocity() {
            if (event.type == ECXEventType::EntityChangeVelocity) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }

        glm::vec3 getNewAngularVelocity() {
            if (event.type == ECXEventType::EntityChangeAngularVelocity) {
                try {
                    return std::any_cast<glm::vec3>(event.args[0]);
                }
                catch (const std::bad_any_cast&) {
                    return glm::vec3(0);
                }
            }
            return glm::vec3(0);
        }

        // Access to game (for getting other entities, etc)
        std::shared_ptr<GameEntity> getGameEntity(const std::string& name) {
            return game ? game->getEntityByName(name) : nullptr;
        }
    };

    void registerAPI() {
        luabridge::getGlobalNamespace(m_luaState)
            .beginClass<EntityAPI>("Entity")
            .addFunction("getName", &EntityAPI::getName)
            .addFunction("getUID", &EntityAPI::getUID)
            .addFunction("isActive", &EntityAPI::isActive)
            .addFunction("activate", &EntityAPI::activate)
            .addFunction("deactivate", &EntityAPI::deactivate)
            .addFunction("getPosition", &EntityAPI::getPosition)
            .addFunction("setPosition", &EntityAPI::setPosition)
            .addFunction("getVelocity", &EntityAPI::getVelocity)
            .addFunction("setVelocity", &EntityAPI::setVelocity)
            .addFunction("getOrientation", &EntityAPI::getOrientation)
            .addFunction("setOrientation", &EntityAPI::setOrientation)
            .addFunction("getAngularVelocity", &EntityAPI::getAngularVelocity)
            .addFunction("setAngularVelocity", &EntityAPI::setAngularVelocity)
            .addFunction("getForward", &EntityAPI::getForward)
            .addFunction("getUp", &EntityAPI::getUp)
            .addFunction("getRight", &EntityAPI::getRight)
            .addFunction("moveForward", &EntityAPI::moveForward)
            .addFunction("moveBack", &EntityAPI::moveBack)
            .addFunction("moveLeft", &EntityAPI::moveLeft)
            .addFunction("moveRight", &EntityAPI::moveRight)
            .addFunction("moveUp", &EntityAPI::moveUp)
            .addFunction("moveDown", &EntityAPI::moveDown)
            .addFunction("rotateAroundAxis", &EntityAPI::rotateAroundAxis)
            .addFunction("setFloat", &EntityAPI::setFloat)
            .addFunction("getFloat", &EntityAPI::getFloat)
            .addFunction("setString", &EntityAPI::setString)
            .addFunction("getString", &EntityAPI::getString)
            .endClass()

            .beginClass<EventAPI>("Event")
            .addFunction("getKey", &EventAPI::getKey)
            .addFunction("isPressed", &EventAPI::isPressed)
            .addFunction("isHeld", &EventAPI::isHeld)
            .addFunction("isReleased", &EventAPI::isReleased)
            .addFunction("getMouseMotionX", &EventAPI::getMouseMotionX)
            .addFunction("getMouseMotionY", &EventAPI::getMouseMotionY)
            .addFunction("getMouseButton", &EventAPI::getMouseButton)
            .addFunction("mouseButtonPressed", &EventAPI::mouseButtonPressed)
            .addFunction("mouseButtonHeld", &EventAPI::mouseButtonHeld)
            .addFunction("mouseButtonReleased", &EventAPI::mouseButtonReleased)
            .addFunction("getOtherEntityUID", &EventAPI::getOtherEntityUID)
            .addFunction("getNewPosition", &EventAPI::getNewPosition)
            .addFunction("getNewOrientation", &EventAPI::getNewOrientation)
            .addFunction("getNewVelocity", &EventAPI::getNewVelocity)
            .addFunction("getNewAngularVelocity", &EventAPI::getNewAngularVelocity)
            .endClass()

            .beginClass<glm::vec3>("vec3")
            .addConstructor<void(*)(float, float, float)>()
            .addProperty("x", &glm::vec3::x)
            .addProperty("y", &glm::vec3::y)
            .addProperty("z", &glm::vec3::z)
            .endClass();
            //.beginClass<EC_Game>("game")
            //.addFunction("getEntity", &EC_Game::getEntityByName)
            //.addFunction("getKeyState", &EC_Game::getKeyState)
            //.addFunction("shutdown", &EC_Game::shutDown)
            //.endClass();
        // Global game reference (as pointer)
       // luabridge::push(m_luaState, m_game);
        //lua_setglobal(m_luaState, "game");
    }
};