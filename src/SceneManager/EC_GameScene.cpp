#include "EC_GameScene.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"

void EC_GameScene::activate()
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    for (EntityID entity : m_Entities)
    {
        if (manager.isAlive(entity) && manager.hasComponent<EC_DOD_EntityInfo>(entity))
            manager.getComponent<EC_DOD_EntityInfo>(entity).active = true;
    }
}

void EC_GameScene::deactivate()
{
    auto& manager = EC_DOD_EntityManager::getInstance();
    for (EntityID entity : m_Entities)
    {
        if (manager.isAlive(entity) && manager.hasComponent<EC_DOD_EntityInfo>(entity))
            manager.getComponent<EC_DOD_EntityInfo>(entity).active = false;
    }
}