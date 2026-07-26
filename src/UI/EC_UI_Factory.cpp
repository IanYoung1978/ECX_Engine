#include "EC_UI_Factory.h"
#include "EC_UI_Components.h"
#include "Components/EC_DOD_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Messaging/ECXMessenger.h"
#include "Logging/ECX_Logging.h"
#include "xml/tinyxml.h"
#include <sstream>
#include <unordered_map>

namespace
{
    glm::vec2 parseVec2(const std::string& text)
    {
        glm::vec2 result(0.0f);
        std::stringstream ss(text);
        char delim;
        ss >> result.x >> delim >> result.y;
        return result;
    }

    glm::vec4 parseColour(const std::string& text)
    {
        glm::vec4 result(0.0f, 0.0f, 0.0f, 1.0f);
        std::stringstream ss(text);
        char delim;
        ss >> result.r >> delim >> result.g >> delim >> result.b >> delim >> result.a;
        return result;
    }

    ECXEventType getUIEventType(const std::string& name)
    {
        static const std::unordered_map<std::string, ECXEventType> handlerMap = {
            {"OnClick",       ECXEventType::click},
            {"OnMouseEnter",  ECXEventType::mouse_enter},
            {"OnMouseLeave",  ECXEventType::mouse_leave},
            {"OnSelect",      ECXEventType::select},
            {"OnUnSelect",    ECXEventType::unselect},
            {"OnUpdate",      ECXEventType::system_update},
        };
        auto it = handlerMap.find(name);
        return (it != handlerMap.end()) ? it->second : ECXEventType::None;
    }

    void parseUIScript(TiXmlElement* elem, EntityID entity)
    {
        auto& manager = EC_DOD_EntityManager::getInstance();
        EC_DOD_ScriptData script;

        auto child = elem->FirstChildElement();
        while (child != nullptr)
        {
            ECXEventType eventType = getUIEventType(child->Value());
            if (eventType != ECXEventType::None)
            {
                const char* filenameAttr = child->Attribute("filename");
                if (filenameAttr)
                    script.handlers[eventType] = filenameAttr;
            }
            child = child->NextSiblingElement();
        }

        manager.addComponent(entity, script);
    }

    void parseUIElement(TiXmlElement* elem, EntityID parent)
    {
        auto& manager = EC_DOD_EntityManager::getInstance();

        EntityID entity = manager.createEntity();

        EC_DOD_EntityInfo info;
        const char* nameAttr = elem->Attribute("name");
        if (nameAttr) info.name = nameAttr;
        manager.addComponent(entity, info);

        EC_UI_Element uiElement;
        double x = 0.0, y = 0.0, w = 0.0, h = 0.0;
        int layer = 0;
        elem->QueryDoubleAttribute("x", &x);
        elem->QueryDoubleAttribute("y", &y);
        elem->QueryDoubleAttribute("width", &w);
        elem->QueryDoubleAttribute("height", &h);
        elem->QueryIntAttribute("layer", &layer);
        uiElement.position = glm::vec2((float)x, (float)y);
        uiElement.size = glm::vec2((float)w, (float)h);
        uiElement.layer = layer;
        const char* visibleAttr = elem->Attribute("visible");
        uiElement.visible = !visibleAttr || (std::string(visibleAttr) == "true");
        manager.addComponent(entity, uiElement);

        if (parent != INVALID_ENTITY)
        {
            if (!manager.hasComponent<EC_DOD_Hierarchy>(parent))
                manager.addComponent(parent, EC_DOD_Hierarchy{});
            EC_DOD_Hierarchy hierarchy;
            hierarchy.parent = parent;
            hierarchy.depth = manager.getComponent<EC_DOD_Hierarchy>(parent).depth + 1;
            manager.addComponent(entity, hierarchy);
            manager.getComponent<EC_DOD_Hierarchy>(parent).children.push_back(entity);
        }

        auto child = elem->FirstChildElement();
        while (child != nullptr)
        {
            std::string tag = child->Value();
            if (tag == "UIPanel")
            {
                EC_UI_Panel panel;
                const char* colourAttr = child->Attribute("colour");
                if (colourAttr) panel.colour = parseColour(colourAttr);
                manager.addComponent(entity, panel);
            }
            else if (tag == "UIText")
            {
                EC_UI_Text text;
                const char* colourAttr = child->Attribute("colour");
                if (colourAttr) text.colour = parseColour(colourAttr);
                if (child->GetText()) text.text = child->GetText();
                manager.addComponent(entity, text);
            }
            else if (tag == "ScriptComponent")
            {
                parseUIScript(child, entity);
            }
            else if (tag == "UIElement")
            {
                parseUIElement(child, entity);
            }
            else
            {
                LOGGING::ECX_Logger::GetInstance()->LogMessage(
                    "EC_UI_Factory: unknown UI element property: " + tag,
                    LOGGING::LogLevel::WARNING);
            }
            child = child->NextSiblingElement();
        }
    }
}

namespace EC_UI_Factory
{
    bool loadUI(const std::string& filename, ECXMessenger& messenger)
    {
        TiXmlDocument doc(filename.c_str());
        if (!doc.LoadFile())
        {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "EC_UI_Factory: failed to load " + filename, LOGGING::LogLevel::SEVERE);
            return false;
        }

        auto root = doc.FirstChildElement();
        if (!root || strcmp(root->Value(), "UI") != 0)
        {
            LOGGING::ECX_Logger::GetInstance()->LogMessage(
                "EC_UI_Factory: " + filename + " missing root <UI> element", LOGGING::LogLevel::SEVERE);
            return false;
        }

        size_t count = 0;
        auto child = root->FirstChildElement("UIElement");
        while (child != nullptr)
        {
            parseUIElement(child, INVALID_ENTITY);
            count++;
            child = child->NextSiblingElement("UIElement");
        }

        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_UI_Factory: loaded " + std::to_string(count) + " top-level UI elements from " + filename,
            LOGGING::LogLevel::INFORMATION);
        return true;
    }
}
