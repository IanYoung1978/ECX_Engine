#include "EC_UI_InputSystem.h"
#include "EC_UI_Components.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include "Game.h"
#include "Engine/Controllers/MouseButton.h"
#include "Messaging/ECXMessenger.h"
#include "Messaging/ECXEvent.h"
#include "Messaging/ECXEventType.h"
#include <typeindex>

namespace
{
    EntityID findTopmostUIElementAt(const glm::vec2& point)
    {
        auto& manager = EC_DOD_EntityManager::getInstance();
        auto entities = manager.getEntitiesWithComponents({
            std::type_index(typeid(EC_UI_Element))
            });

        EntityID best = INVALID_ENTITY;
        int bestLayer = 0;
        for (EntityID entity : entities)
        {
            // Purely decorative elements (no ScriptComponent, e.g. a text label sitting
            // on top of its parent button) aren't hit-test candidates - they'd otherwise
            // shadow the interactive element beneath them since they're drawn (and thus
            // hit-tested) at a higher layer.
            if (!manager.hasComponent<EC_DOD_ScriptData>(entity)) continue;

            const auto& element = manager.getComponent<EC_UI_Element>(entity);
            if (!element.visible) continue;

            glm::vec2 pos = ResolveUIAbsolutePosition(entity);
            if (point.x < pos.x || point.x > pos.x + element.size.x ||
                point.y < pos.y || point.y > pos.y + element.size.y)
                continue;

            if (best == INVALID_ENTITY || element.layer >= bestLayer)
            {
                best = entity;
                bestLayer = element.layer;
            }
        }
        return best;
    }

    void publishTargeted(ECXMessenger& messenger, ECXEventType type, EntityID target)
    {
        if (target == INVALID_ENTITY) return;
        ECXEvent event;
        event.type = type;
        event.targetEntity = target;
        messenger.publish(event);
    }
}

void EC_UI_InputSystem::update(EC_Game& game, ECXMessenger& messenger)
{
    glm::ivec2 mousePos = game.getMousePosition();
    EntityID hovered = findTopmostUIElementAt(glm::vec2(mousePos));

    if (hovered != m_HoveredEntity)
    {
        publishTargeted(messenger, ECXEventType::mouse_leave, m_HoveredEntity);
        publishTargeted(messenger, ECXEventType::mouse_enter, hovered);
        m_HoveredEntity = hovered;
    }

    if (hovered != INVALID_ENTITY && game.isMouseButtonPressed(MouseButton::LMB))
    {
        publishTargeted(messenger, ECXEventType::click, hovered);

        if (hovered != m_SelectedEntity)
        {
            publishTargeted(messenger, ECXEventType::unselect, m_SelectedEntity);
            publishTargeted(messenger, ECXEventType::select, hovered);
            m_SelectedEntity = hovered;
        }
    }
}
