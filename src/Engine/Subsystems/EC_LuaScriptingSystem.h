#pragma once
#include <lua.hpp>
#include <luabridge3/LuaBridge/LuaBridge.h>
#include <string>
#include <memory>
#include <unordered_map>
#include <any>
#include "Messaging/ECXEvent.h"
#include "Messaging/KeyEvent.h"
#include "Messaging/MouseEvent.h"
#include "Components/EC_ScriptComponent.h"
#include "Engine/Subsystems/EC_System.h"
#include "Messaging/IEventListener.h"
#include "Game.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Logging/ECX_Logging.h"
#include <fstream>
#include <iostream>
#include "Engine/Subsystems/Scripting/EC_ScriptAPI.h"

class EC_LuaScriptSystem : public EC_System, public IEventListener{
public:
    EC_LuaScriptSystem() : m_luaState(nullptr), m_game(nullptr) {}

    ~EC_LuaScriptSystem() {
        if (m_luaState) lua_close(m_luaState);
        delete m_game;
        m_game = nullptr;
    }

    void init(ECXMessenger & messenger, EC_Game & game) override {
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

        m_game = new ScriptAPI::GameAPI(&game);

        // Create shared Lua state
        m_luaState = luaL_newstate();
        luaL_openlibs(m_luaState);
        registerAPI();
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Scripting system initialised",
            LOGGING::LogLevel::INFORMATION
        );
    }

    void update(const float& deltaTimeS, EC_Game & game) override {
        auto& manager = EC_DOD_EntityManager::getInstance();

        auto entities = manager.getEntitiesWithComponent(
            std::type_index(typeid(EC_DOD_ScriptData))
        );

        for (EntityID entity : entities) {
            if (!manager.isAlive(entity)) continue;

            const auto& script = manager.getComponent<EC_DOD_ScriptData>(entity);
            if (!script.enabled) continue;

            callLuaFunction(script.scriptFile, "update", entity, deltaTimeS);
        }
    }

    void receive(ECXEvent & event) override {
        auto& manager = EC_DOD_EntityManager::getInstance();

        auto entities = manager.getEntitiesWithComponent(
            std::type_index(typeid(EC_DOD_ScriptData))
        );

        for (EntityID entity : entities) {
            if (!manager.isAlive(entity)) continue;

            const auto& script = manager.getComponent<EC_DOD_ScriptData>(entity);
            if (!script.enabled) continue;

            // Route event to appropriate Lua function
            const char* funcName = getEventFunctionName(event.type);
            if (funcName) {
                callLuaEvent(script.scriptFile, funcName, entity, event);
            }
        }
    }

private:
    lua_State * m_luaState;
    ScriptAPI::GameAPI* m_game;
    std::unordered_map<std::string, bool> m_loadedScripts;

    // Map event types to Lua function names
    const char* getEventFunctionName(ECXEventType type) {
        switch (type) {
        case ECXEventType::EntityCreate:    return "onEntityCreate";
        case ECXEventType::EntityKill:      return "onEntityKill";
        case ECXEventType::EntityDestroy:   return "onEntityDestroy";
        case ECXEventType::entity_loaded:   return "onEntityLoaded";
        case ECXEventType::EntityStopRotation:           return "onStopRotation";
        case ECXEventType::EntityStopMotion:             return "onStopMotion";
        case ECXEventType::EntityChangePosition:         return "onPositionChanged";
        case ECXEventType::EntityChangeOrientation:      return "onOrientationChanged";
        case ECXEventType::EntityChangeAngularVelocity:  return "onAngularVelocityChanged";
        case ECXEventType::EntityChangeVelocity:         return "onVelocityChanged";
        case ECXEventType::CollisionBeginEvent: return "onCollisionBegin";
        case ECXEventType::CollisionEndEvent:   return "onCollisionEnd";
        case ECXEventType::key_down:    return "onKeyDown";
        case ECXEventType::key_up:      return "onKeyUp";
        case ECXEventType::key_held:    return "onKeyHeld";
        case ECXEventType::mouse_down:  return "onMouseDown";
        case ECXEventType::mouse_up:    return "onMouseUp";
        case ECXEventType::mouse_held:  return "onMouseHeld";
        case ECXEventType::mouse_move:  return "onMouseMove";
        case ECXEventType::world_loaded:    return "onWorldLoaded";
        case ECXEventType::config_loaded:   return "onConfigLoaded";
        case ECXEventType::system_update:   return "onSystemUpdate";
        default: return nullptr;
        }
    }

    bool loadScript(const std::string& filename) {
        if (m_loadedScripts[filename]) return true;

        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Attempting to load script: " + filename,
            LOGGING::LogLevel::INFORMATION
        );

        int result = luaL_dofile(m_luaState, filename.c_str());
        if (result != LUA_OK) {
            const char* error = lua_tostring(m_luaState, -1);
            std::string errorMsg = error ? error : "Unknown error";
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "Lua error loading " + filename + ": " + errorMsg +
                " (error code: " + std::to_string(result) + ")",
                LOGGING::LogLevel::SEVERE
            );
            lua_pop(m_luaState, 1);
            return false;
        }

        m_loadedScripts[filename] = true;
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Script loaded successfully: " + filename,
            LOGGING::LogLevel::INFORMATION
        );
        return true;
    }

    void callLuaFunction(const std::string& scriptFile, const char* funcName,
                        EntityID entity, float deltaTime) {
        if (!loadScript(scriptFile)) return;

        try {
            luabridge::LuaRef func = luabridge::getGlobal(m_luaState, funcName);
            if (func.isFunction()) {
                ScriptAPI::EntityAPI entityAPI(entity);
                auto result = func(entityAPI, deltaTime);

                if (!result) {
                    LOGGING::ECX_Logger::GetInstance()->LogMessage(
                        "Error in " + std::string(funcName) + ": " + result.errorMessage(),
                        LOGGING::LogLevel::WARNING
                    );
                }
            }
        }
        catch (std::exception& e) {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "Exception calling " + std::string(funcName) + ": " + e.what(),
                LOGGING::LogLevel::SEVERE
            );
        }
    }

    void callLuaEvent(const std::string& scriptFile, const char* funcName,
                     EntityID entity, ECXEvent& event) {
        if (!loadScript(scriptFile)) return;

        try {
            luabridge::LuaRef func = luabridge::getGlobal(m_luaState, funcName);
            if (func.isFunction()) {
                ScriptAPI::EntityAPI entityAPI(entity);
                ScriptAPI::EventAPI eventAPI(event, m_game->game);
                auto result = func(entityAPI, eventAPI);

                if (!result) {
                    LOGGING::ECX_Logger::GetInstance()->LogMessage(
                        "Error in " + std::string(funcName) + ": " + result.errorMessage(),
                        LOGGING::LogLevel::WARNING
                    );
                }
            }
        }
        catch (std::exception& e) {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "Exception calling " + std::string(funcName) + ": " + e.what(),
                LOGGING::LogLevel::SEVERE
            );
        }
    }

    void registerAPI() {
        luabridge::getGlobalNamespace(m_luaState)
            .beginClass<ScriptAPI::EntityAPI>("Entity")
            .addFunction("getName", &ScriptAPI::EntityAPI::getName)
            .addFunction("getUID", &ScriptAPI::EntityAPI::getUID)
            .addFunction("isActive", &ScriptAPI::EntityAPI::isActive)
            .addFunction("activate", &ScriptAPI::EntityAPI::activate)
            .addFunction("deactivate", &ScriptAPI::EntityAPI::deactivate)
            .addFunction("getPosition", &ScriptAPI::EntityAPI::getPosition)
            .addFunction("setPosition", &ScriptAPI::EntityAPI::setPosition)
            .addFunction("getVelocity", &ScriptAPI::EntityAPI::getVelocity)
            .addFunction("setVelocity", &ScriptAPI::EntityAPI::setVelocity)
            .addFunction("getOrientation", &ScriptAPI::EntityAPI::getOrientation)
            .addFunction("setOrientation", &ScriptAPI::EntityAPI::setOrientation)
            .addFunction("getAngularVelocity", &ScriptAPI::EntityAPI::getAngularVelocity)
            .addFunction("setAngularVelocity", &ScriptAPI::EntityAPI::setAngularVelocity)
            .addFunction("getForward", &ScriptAPI::EntityAPI::getForward)
            .addFunction("getUp", &ScriptAPI::EntityAPI::getUp)
            .addFunction("getRight", &ScriptAPI::EntityAPI::getRight)
            .addFunction("moveForward", &ScriptAPI::EntityAPI::moveForward)
            .addFunction("moveBack", &ScriptAPI::EntityAPI::moveBack)
            .addFunction("moveLeft", &ScriptAPI::EntityAPI::moveLeft)
            .addFunction("moveRight", &ScriptAPI::EntityAPI::moveRight)
            .addFunction("moveUp", &ScriptAPI::EntityAPI::moveUp)
            .addFunction("moveDown", &ScriptAPI::EntityAPI::moveDown)
            .addFunction("rotateAroundAxis", &ScriptAPI::EntityAPI::rotateAroundAxis)
            .addFunction("setFloat", &ScriptAPI::EntityAPI::setFloat)
            .addFunction("getFloat", &ScriptAPI::EntityAPI::getFloat)
            .addFunction("setString", &ScriptAPI::EntityAPI::setString)
            .addFunction("getString", &ScriptAPI::EntityAPI::getString)
            .endClass()

            .beginClass<ScriptAPI::EventAPI>("Event")
            .addFunction("getKey", &ScriptAPI::EventAPI::getKey)
            .addFunction("isPressed", &ScriptAPI::EventAPI::isPressed)
            .addFunction("isHeld", &ScriptAPI::EventAPI::isHeld)
            .addFunction("isReleased", &ScriptAPI::EventAPI::isReleased)
            .addFunction("getMouseMotionX", &ScriptAPI::EventAPI::getMouseMotionX)
            .addFunction("getMouseMotionY", &ScriptAPI::EventAPI::getMouseMotionY)
            .addFunction("getMouseButton", &ScriptAPI::EventAPI::getMouseButton)
            .addFunction("mouseButtonPressed", &ScriptAPI::EventAPI::mouseButtonPressed)
            .addFunction("mouseButtonHeld", &ScriptAPI::EventAPI::mouseButtonHeld)
            .addFunction("mouseButtonReleased", &ScriptAPI::EventAPI::mouseButtonReleased)
            .addFunction("getOtherEntityUID", &ScriptAPI::EventAPI::getOtherEntityUID)
            .addFunction("getNewPosition", &ScriptAPI::EventAPI::getNewPosition)
            .addFunction("getNewOrientation", &ScriptAPI::EventAPI::getNewOrientation)
            .addFunction("getNewVelocity", &ScriptAPI::EventAPI::getNewVelocity)
            .addFunction("getNewAngularVelocity", &ScriptAPI::EventAPI::getNewAngularVelocity)
            .endClass()

            .beginClass<glm::vec3>("vec3")
            .addConstructor<void(*)(float, float, float)>()
            .addProperty("x", &glm::vec3::x)
            .addProperty("y", &glm::vec3::y)
            .addProperty("z", &glm::vec3::z)
            .endClass()

            .beginClass<ScriptAPI::GameAPI>("game")
            .addFunction("getEntity", &ScriptAPI::GameAPI::getEntity)
            .addFunction("getKeyState", &ScriptAPI::GameAPI::getKeyState)
            .addFunction("shutdown", &ScriptAPI::GameAPI::shutdown)
            .endClass();

        // Global game reference (as pointer)
        luabridge::push(m_luaState, m_game);
        lua_setglobal(m_luaState, "game");
    }
};