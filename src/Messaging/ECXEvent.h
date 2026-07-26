#pragma once
#include "ECXEventType.h"
#include "Entity/EC_DOD_Types.h"
#include <any>

class ECXEvent
{
public:
	ECXEventType type;
	std::any args[8];
	// When set (not INVALID_ENTITY), only this entity's matching handler fires - used by UI
	// interaction events (mouse_enter/leave/select/unselect/click) so a click on one element
	// doesn't fire every other element's handler too. Unset (the default) preserves the
	// existing broadcast-to-all-subscribers behavior every other event type already relies on.
	EntityID targetEntity = INVALID_ENTITY;
};

