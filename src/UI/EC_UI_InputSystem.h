#pragma once
#include "Entity/EC_DOD_Types.h"

class EC_Game;
class ECXMessenger;

// Per-frame hit-testing over EC_UI_Element entities: tracks hover/selection state across
// frames and publishes targeted mouse_enter/mouse_leave/click/select/unselect ECXEvents
// (see ECXEvent::targetEntity) so only the element actually under the cursor reacts.
class EC_UI_InputSystem
{
public:
    void update(EC_Game& game, ECXMessenger& messenger);

private:
    EntityID m_HoveredEntity = INVALID_ENTITY;
    EntityID m_SelectedEntity = INVALID_ENTITY;
};
