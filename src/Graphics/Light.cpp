#include "Light.h"



Light::Light()
{
	m_CastsShadow = false;
	m_LightType = LightType::Directional;
	m_Static = true;
}


Light::~Light()
{
}

ComponentType Light::getComponentType()
{
	return ComponentType::Light;
}


void Light::setLightData(std::shared_ptr<LightData> data)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_Data = data;
}

void Light::setLightType(LightType type)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_LightType = type;
}

void Light::setShadowCaster(bool caster)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_CastsShadow = caster;
}

void Light::setDynamic()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_Static = false;
}

void Light::setStatic()
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_Static = true;
}

void Light::setShadowState(bool state)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	m_CastsShadow = state;
}

bool Light::castsShadow()
{
	return m_CastsShadow;
}

bool Light::dynamic()
{
	return !m_Static;
}

LightType Light::getLightType()
{
	return m_LightType;
}

LightData & Light::getLightData()
{
	return (*m_Data);
}
