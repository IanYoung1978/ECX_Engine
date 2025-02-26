#pragma once
enum class ComponentType
{
	Spatial, //position, rotation, motion related
	Transform,
	GraphicsData, //textures, models, shaders etc
	Camera,
	Light,
	Script,
	Body,
	CollisionBehaviour,
	CharacterData,
	GUI_Data,
	SkyBox,
	Num_Types
};