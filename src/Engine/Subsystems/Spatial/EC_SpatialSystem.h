#pragma once
#include "Engine/Subsystems/EC_System.h"

class EC_SpatialSystem : public EC_System {
public:
    EC_SpatialSystem();
    virtual ~EC_SpatialSystem();

    virtual void init(ECXMessenger& messenger, EC_Game& game) override;
    virtual void update(const float& deltaTimeS, EC_Game& game) override;
};