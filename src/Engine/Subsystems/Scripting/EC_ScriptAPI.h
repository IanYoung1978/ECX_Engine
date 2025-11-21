#pragma once


namespace ScriptAPI
{
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
    struct GameAPI {
        EC_Game* game;
        GameAPI(EC_Game* g) : game(g) {}
        EntityAPI getEntity(const std::string& name) {
            return game ? game->getEntityByName(name) : nullptr;
            auto e = game->getEntityByName(name);
            if (e != nullptr)
            {
                return EntityAPI(e);
            }
            return nullptr;
        }
        void shutdown() {
            if (game) game->shutDown();
        }
        int getKeyState(const std::string& key) {
            if (game) {
                SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());
                KeyState state = game->getKeyState(scancode);
                return static_cast<int>(state);
            }
            return static_cast<int>(KeyState::None);
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
}