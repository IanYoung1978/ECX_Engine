#include "EC_LuaScriptProxy.h"
#include "Spatial.h"
#include "EC_ScriptComponent.h"
#include <lua.hpp>
#include <LuaBridge3/LuaBridge/LuaBridge.h>
#include "Entity/GameEntity.h"
#include "Messaging/KeyEvent.h"


EC_LuaScriptProxy::EC_LuaScriptProxy()
{
	entity_ID = 0;
}

EC_LuaScriptProxy::EC_LuaScriptProxy(std::shared_ptr<GameEntity> e)
{
	entity_ID = e->getUID();
	m_entity = e;
	m_spatial = e->getComponent<Spatial>();
	m_scripts = e->getComponent<EC_ScriptComponent>();
}

EC_LuaScriptProxy::EC_LuaScriptProxy(const EC_LuaScriptProxy & proxy)
{
	m_entity = proxy.m_entity;
	m_scripts = proxy.m_scripts;
	m_spatial = proxy.m_spatial;
	entity_ID = proxy.entity_ID;
}


EC_LuaScriptProxy::~EC_LuaScriptProxy()
{
}
Vec3 EC_LuaScriptProxy::getPosition() const
{
	return Vec3(m_spatial->getPosition());
}

void EC_LuaScriptProxy::setPosition(const Vec3 & position)
{
	m_spatial->setPosition(Vec3::get(position));
}

Vec3 EC_LuaScriptProxy::getVelocity() const
{
	return Vec3(m_spatial->getVelocity());
}

void EC_LuaScriptProxy::setVelocity(const Vec3 & velocity)
{
	m_spatial->setVelocity(Vec3::get(velocity));
}

Vec3 EC_LuaScriptProxy::getOrientation() const
{
	return Vec3(m_spatial->getOrientation());
}

void EC_LuaScriptProxy::setOrientation(const Vec3 & orientation)
{
	m_spatial->setOrientation(Vec3::get(orientation));
}

Vec3 EC_LuaScriptProxy::getRotation() const
{
	return Vec3(m_spatial->getAngVelocity());
}

void EC_LuaScriptProxy::setRotation(const Vec3 & rotation)
{
	m_spatial->setAngVelocity(Vec3::get(rotation));
}

Vec3 EC_LuaScriptProxy::getForward() const
{
	return Vec3(m_spatial->getForward());
}

Vec3 EC_LuaScriptProxy::getUp() const
{
	return Vec3(m_spatial->getUp());
}

Vec3 EC_LuaScriptProxy::getRight() const
{
	return Vec3(m_spatial->getRight());
}

void EC_LuaScriptProxy::moveForward(const float & distance)
{
	glm::vec3 d = m_spatial->getForward()*distance;
	m_spatial->setPosition(m_spatial->getPosition() + d);
}

void EC_LuaScriptProxy::moveBack(const float & distance)
{
	glm::vec3 d = m_spatial->getForward()*distance;
	m_spatial->setPosition(m_spatial->getPosition() - d);
}

void EC_LuaScriptProxy::moveLeft(const float & distance)
{
	glm::vec3 d = m_spatial->getRight()*distance;
	m_spatial->setPosition(m_spatial->getPosition() - d);
}

void EC_LuaScriptProxy::moveRight(const float & distance)
{
	glm::vec3 d = m_spatial->getRight()*distance;
	m_spatial->setPosition(m_spatial->getPosition() + d);
}

void EC_LuaScriptProxy::moveUp(const float & distance)
{
	glm::vec3 d = m_spatial->getUp()*distance;
	m_spatial->setPosition(m_spatial->getPosition() + d);
}

void EC_LuaScriptProxy::moveDown(const float & distance)
{
	glm::vec3 d = m_spatial->getUp()*distance;
	m_spatial->setPosition(m_spatial->getPosition() - d);
}

void EC_LuaScriptProxy::rotateAroundAxis(const float& rotation, const std::string& axis)
{
	auto spat = m_entity->getComponent<Spatial>();
	glm::vec3 o = spat->getOrientation();
	if (axis == "X")
	{
		o.x += rotation;
	}
	if (axis == "Y")
	{
		o.y += rotation;
	}
	if (axis == "Z")
	{
		o.z += rotation;
	}
	spat->setOrientation(o);
}

void EC_LuaScriptProxy::handleEvent(ECXEvent& e, EC_Game & game)
{
	m_scripts->handleEvent(e, game, *this);
}