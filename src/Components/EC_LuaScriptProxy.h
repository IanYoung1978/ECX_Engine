#pragma once
#include <glm\glm.hpp>
#include <memory>
#include "Game.h"
#include "Messaging/ECXEvent.h"

class Spatial;
class EC_ScriptComponent;
// a proxy class for glm::vec3 used for exposing common vector data and functions to lua
class Vec3
{
public:
	Vec3(const float& ix, const float& iy, const float& iz) :
		x(ix),
		y(iy),
		z(iz)
	{

	}
	Vec3(glm::vec3 vec) {
		x = vec.x;
		y = vec.y;
		z = vec.z;
	}
	Vec3():x(0.0f),y(0.0f),z(0.0f) {}
	Vec3(const Vec3& v) {
		x = v.x;
		y = v.y;
		z = v.z;
	}
	Vec3 operator*=(const float& scalar) {
		Vec3 vec(*this);
		vec.x *= scalar;
		vec.y *= scalar;
		vec.z *= scalar;
		return vec;
	}
	Vec3 operator/=(const float& scalar) {
		Vec3 vec(*this);
		float div = 1.0f / scalar;
		vec *= (div);
		return vec;
	}
	Vec3 operator+=(const Vec3& v)
	{
		Vec3 vec(v);
		vec.x += x;
		vec.y += y;
		vec.z += z;
		return vec;
	}
	Vec3 operator-=(const Vec3& v)
	{
		Vec3 vec(v);
		vec.x -= x;
		vec.y -= y;
		vec.z -= z;
		return vec;
	}
	static float length(const Vec3& v)
	{
		float l = sqrt((v.x*v.x) + (v.y*v.y)*(v.z*v.z));
		return l;
	}
	static Vec3 normalize(const Vec3& v)
	{
		Vec3 vec(v);
		vec /= length(v);
		return v;
	}
	static glm::vec3 get(const Vec3& v)
	{
		return glm::vec3(v.x, v.y, v.z);
	}
	float x, y, z;
};

class EC_LuaScriptProxy
{
public:
	EC_LuaScriptProxy();
	EC_LuaScriptProxy(std::shared_ptr<GameEntity> e);
	EC_LuaScriptProxy(const EC_LuaScriptProxy& proxy);
	~EC_LuaScriptProxy();
	//bindable functions
	Vec3 getPosition() const;
	void setPosition(const Vec3& position);
	Vec3 getVelocity() const;
	void setVelocity(const Vec3& velocity);
	Vec3 getOrientation() const;
	void setOrientation(const Vec3& orientation);
	Vec3 getRotation() const;
	void setRotation(const Vec3& rotation);
	Vec3 getForward() const;
	Vec3 getUp() const;
	Vec3 getRight() const;
	void moveForward(const float& distance);
	void moveBack(const float& distance);
	void moveLeft(const float& distance);
	void moveRight(const float& distance);
	void moveUp(const float& distance);
	void moveDown(const float& distance);
	void rotateAroundAxis(const float& rotation, const std::string& axis);
	void handleEvent(ECXEvent& e, EC_Game & game);
	std::shared_ptr<EC_ScriptComponent> m_scripts;
	std::shared_ptr<Spatial> m_spatial;
	unsigned int entity_ID;
	std::shared_ptr<GameEntity> m_entity;
};


