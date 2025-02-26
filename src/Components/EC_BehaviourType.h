#pragma once
enum class EC_BehaviourType
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
	mouse_move,
};