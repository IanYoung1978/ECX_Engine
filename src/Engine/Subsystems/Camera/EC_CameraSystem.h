#pragma once
#include "Engine/Subsystems/EC_System.h"

class EC_CameraSystem : public EC_System {
public:
    EC_CameraSystem();
    virtual ~EC_CameraSystem();

    virtual void init(ECXMessenger& messenger, EC_Game& game) override;
    virtual void update(const float& deltaTimeS, EC_Game& game) override;
};