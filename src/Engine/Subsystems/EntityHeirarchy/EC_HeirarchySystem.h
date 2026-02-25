#pragma once
#include "Engine/Subsystems/EC_System.h"
#include "Messaging/ICommandListener.h"
#include "Messaging/IEventListener.h"
#include "Messaging/ECXEventType.h"
#include "Messaging/ECXCommandType.h"
#include "Entity/EC_DOD_Types.h"


class EC_HierarchySystem : public EC_System, public ICommandListener, public IEventListener {
public:
    EC_HierarchySystem() = default;
    ~EC_HierarchySystem() = default;

    void init(ECXMessenger& messenger, EC_Game& game) override;
    void update(const float& deltaTimeS, EC_Game& game) override;
    void receive(ECXCommand& command) override;
    void receive(ECXEvent& event) override;

private:
    void setParent(EntityID child, EntityID newParent);
    void clearParent(EntityID child);
    void removeFromParent(EntityID child);
    void addToParent(EntityID child, EntityID newParent);
    void updateDepth(EntityID entity, uint32_t newDepth);
    void publishHierarchyChanged(EntityID child, EntityID oldParent, EntityID newParent);

    ECXMessenger* m_Messenger = nullptr;
};