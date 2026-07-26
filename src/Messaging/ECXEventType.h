#pragma once

enum class ECXEventType
{
	EntityCreate,
	EntityKill,
	EntityDestroy,
	EntityStopRotation,
	EntityStopMotion,
	EntityChangePosition,
	EntityChangeOrientation,
	EntityChangeAngularVelocity,
	EntityChangeVelocity,
	CollisionBeginEvent,
	CollisionEndEvent,
	key_up,
	key_down,
	key_held,
	mouse_up,
	mouse_down,
	mouse_held,
	mouse_move,
	mouse_enter,
	mouse_leave,
	select,
	unselect,
	click,
	world_loaded,
	entity_loaded,
	config_loaded,
	system_update,
	None,
	ALL
};