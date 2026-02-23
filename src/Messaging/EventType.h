#pragma once
enum class EC_EventType
{
	KeyEvent,
	MouseEvent,
	UpdateEvent,
	CreateEvent,
	DestroyEvent,
	CollisionBegin,
	CollisionEnd,
	NumTypes
};