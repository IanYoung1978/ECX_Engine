#include "EC_Skybox_Component.h"



EC_Skybox_Component::EC_Skybox_Component() :
	m_Owner(nullptr),
	m_Visibility(false),
	m_Opacity(0.0f),
	m_SkyBoxTexture(0)
{
}


EC_Skybox_Component::~EC_Skybox_Component()
{
}

unsigned int EC_Skybox_Component::getSkyBoxTexture()
{
	return m_SkyBoxTexture;
}

void EC_Skybox_Component::setSkyBoxTexture(unsigned int texID)
{
	m_SkyBoxTexture = texID;
}

float EC_Skybox_Component::getOpacity()
{
	return m_Opacity;
}

void EC_Skybox_Component::setOpacity(float opacity)
{
	m_Opacity = opacity;
}

bool EC_Skybox_Component::getVisible()
{
	return m_Visibility;
}

void EC_Skybox_Component::setVisible(bool visible)
{
	m_Visibility = visible;
}

ComponentType EC_Skybox_Component::getComponentType()
{
	return ComponentType::SkyBox;
}
