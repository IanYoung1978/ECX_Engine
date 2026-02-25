#include "Engine/Subsystems/EntityHeirarchy/EC_HeirarchySystem.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"
#include "Messaging/ECXMessenger.h"
#include "Messaging/ECXEvent.h"
#include "Messaging/ECXCommand.h"
#include <algorithm>

void EC_HierarchySystem::init(ECXMessenger& messenger, EC_Game& game) {
    m_Messenger = &messenger;
    messenger.Subscribe(*this, ECXCommandType::EntitySetParent);
    messenger.Subscribe(*this, ECXCommandType::EntityClearParent);
    messenger.Subscribe(*this, ECXEventType::EntityDestroy);
}

void EC_HierarchySystem::update(const float& deltaTimeS, EC_Game& game) {
    // Event/command driven — no per-frame work
}

void EC_HierarchySystem::receive(ECXCommand& command) {
    if (command.type == ECXCommandType::EntitySetParent) {
        EntityID child = std::any_cast<EntityID>(command.args[0]);
        EntityID newParent = std::any_cast<EntityID>(command.args[1]);
        setParent(child, newParent);
    }
    else if (command.type == ECXCommandType::EntityClearParent) {
        EntityID child = std::any_cast<EntityID>(command.args[0]);
        clearParent(child);
    }
}

void EC_HierarchySystem::receive(ECXEvent& event) {
    if (event.type == ECXEventType::EntityDestroy) {
        EntityID entity = std::any_cast<EntityID>(event.args[0]);
        auto& manager = EC_DOD_EntityManager::getInstance();

        if (!manager.hasComponent<EC_DOD_Hierarchy>(entity)) return;

        auto& hierarchy = manager.getComponent<EC_DOD_Hierarchy>(entity);

        // Silently detach from parent — entity is gone
        removeFromParent(entity);

        // Notify for each child — let scripts decide what to do
        std::vector<EntityID> childrenCopy = hierarchy.children;
        for (EntityID child : childrenCopy) {
            if (!manager.hasComponent<EC_DOD_Hierarchy>(child)) continue;
            auto& childHierarchy = manager.getComponent<EC_DOD_Hierarchy>(child);
            childHierarchy.parent = INVALID_ENTITY;
            updateDepth(child, 0);
            publishHierarchyChanged(child, entity, INVALID_ENTITY);
        }
    }
}

void EC_HierarchySystem::setParent(EntityID child, EntityID newParent) {
    auto& manager = EC_DOD_EntityManager::getInstance();

    if (!manager.isAlive(child)) return;
    if (newParent != INVALID_ENTITY && !manager.isAlive(newParent)) return;

    if (!manager.hasComponent<EC_DOD_Hierarchy>(child))
        manager.addComponent(child, EC_DOD_Hierarchy{});

    auto& hierarchy = manager.getComponent<EC_DOD_Hierarchy>(child);
    EntityID oldParent = hierarchy.parent;
    if (oldParent == newParent) return;

    removeFromParent(child);
    hierarchy.parent = newParent;
    addToParent(child, newParent);

    uint32_t newDepth = 0;
    if (newParent != INVALID_ENTITY && manager.hasComponent<EC_DOD_Hierarchy>(newParent))
        newDepth = manager.getComponent<EC_DOD_Hierarchy>(newParent).depth + 1;
    updateDepth(child, newDepth);

    if (manager.hasComponent<EC_DOD_Transform>(child))
        manager.getComponent<EC_DOD_Transform>(child).dirty = true;

    publishHierarchyChanged(child, oldParent, newParent);
}

void EC_HierarchySystem::clearParent(EntityID child) {
    setParent(child, INVALID_ENTITY);
}

void EC_HierarchySystem::removeFromParent(EntityID child) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    if (!manager.hasComponent<EC_DOD_Hierarchy>(child)) return;

    EntityID oldParent = manager.getComponent<EC_DOD_Hierarchy>(child).parent;
    if (oldParent == INVALID_ENTITY) return;
    if (!manager.hasComponent<EC_DOD_Hierarchy>(oldParent)) return;

    auto& children = manager.getComponent<EC_DOD_Hierarchy>(oldParent).children;
    children.erase(std::remove(children.begin(), children.end(), child), children.end());
}

void EC_HierarchySystem::addToParent(EntityID child, EntityID newParent) {
    if (newParent == INVALID_ENTITY) return;
    auto& manager = EC_DOD_EntityManager::getInstance();

    if (!manager.hasComponent<EC_DOD_Hierarchy>(newParent))
        manager.addComponent(newParent, EC_DOD_Hierarchy{});

    manager.getComponent<EC_DOD_Hierarchy>(newParent).children.push_back(child);
}

void EC_HierarchySystem::updateDepth(EntityID entity, uint32_t newDepth) {
    auto& manager = EC_DOD_EntityManager::getInstance();
    if (!manager.hasComponent<EC_DOD_Hierarchy>(entity)) return;

    auto& hierarchy = manager.getComponent<EC_DOD_Hierarchy>(entity);
    hierarchy.depth = newDepth;

    for (EntityID child : hierarchy.children)
        updateDepth(child, newDepth + 1);
}

void EC_HierarchySystem::publishHierarchyChanged(EntityID child, EntityID oldParent, EntityID newParent) {
    ECXEvent evt;
    evt.type = ECXEventType::HeirarchyChanged;
    evt.args[0] = child;
    evt.args[1] = oldParent;
    evt.args[2] = newParent;
    m_Messenger->publish(evt);
}