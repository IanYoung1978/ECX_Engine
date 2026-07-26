#pragma once
#include <string>
#include <glm/glm.hpp>
#include "Entity/EC_DOD_Types.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"

// UI foundation (Issue #6). Marker component: an entity with this is a UI element. Position is
// pixel-space, top-left origin, Y-down, relative to its EC_DOD_Hierarchy parent (if any) - the
// existing hierarchy component is reused rather than inventing a separate UI tree. Layer is the
// draw-order key (0 = furthest back), independent of hierarchy depth - all UI elements are
// flattened and sorted by layer at render time, hierarchy only affects position.
struct EC_UI_Element {
    glm::vec2 position{ 0.0f };
    glm::vec2 size{ 0.0f };
    int layer = 0;
    bool visible = true;
};

// Optional: a flat-colour background quad, sized/positioned by the entity's EC_UI_Element.
struct EC_UI_Panel {
    glm::vec4 colour{ 0.0f, 0.0f, 0.0f, 1.0f };
};

// Optional: a text label, positioned at the entity's EC_UI_Element position. Supports embedded
// '\n' for multi-line content (e.g. a scrolling log panel as a single element).
struct EC_UI_Text {
    std::string text;
    glm::vec4 colour{ 1.0f, 1.0f, 1.0f, 1.0f };
};

// Walks EC_DOD_Hierarchy ancestors (reused, not a separate UI tree) summing each ancestor's
// EC_UI_Element::position, to resolve `entity`'s absolute screen position. Shared by both the
// renderer (GL_Deferred_Renderer::uiPass) and hit-testing (EC_UI_InputSystem) so this logic
// exists exactly once.
inline glm::vec2 ResolveUIAbsolutePosition(EntityID entity)
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    if (!manager.hasComponent<EC_UI_Element>(entity))
        return glm::vec2(0.0f);

    glm::vec2 absPos = manager.getComponent<EC_UI_Element>(entity).position;

    EntityID current = entity;
    while (manager.hasComponent<EC_DOD_Hierarchy>(current))
    {
        EntityID parent = manager.getComponent<EC_DOD_Hierarchy>(current).parent;
        if (parent == INVALID_ENTITY || !manager.isAlive(parent)) break;
        if (!manager.hasComponent<EC_UI_Element>(parent)) break;

        absPos += manager.getComponent<EC_UI_Element>(parent).position;
        current = parent;
    }

    return absPos;
}
