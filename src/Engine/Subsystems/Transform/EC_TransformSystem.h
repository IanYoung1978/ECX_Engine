#pragma once
#include "Engine/Subsystems/EC_System.h"

class EC_TransformSystem : public EC_System {
public:
    EC_TransformSystem();
    virtual ~EC_TransformSystem();

    virtual void init(ECXMessenger& messenger, EC_Game& game) override;
    virtual void update(const float& deltaTimeS, EC_Game& game) override;
};