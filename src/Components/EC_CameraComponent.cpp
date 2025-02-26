#include "EC_CameraComponent.h"



EC_CameraComponent::EC_CameraComponent() :
	mDrawDistance(0.0f),
	mFOVAngle(0.0f),
	mViewPortHeight(0),
	mViewPortWidth(0),
	m_IsActive(false)
{
}


EC_CameraComponent::~EC_CameraComponent()
{
}

void EC_CameraComponent::setDrawDistance(float drawDistance)
{
	mDrawDistance = drawDistance;
	notifyObservers();
}

void EC_CameraComponent::setFOVAngle(float fov_radians)
{
	mFOVAngle = fov_radians;
	notifyObservers();
}

void EC_CameraComponent::setViewPort(int width, int height)
{
	mViewPortWidth = width;
	mViewPortHeight = height;
	notifyObservers();
}

void EC_CameraComponent::setActive(bool toggle)
{
	m_IsActive = toggle;
	notifyObservers();
}

void EC_CameraComponent::setViewMatrix(glm::mat4 & view_matrix)
{
	m_ViewMatrix = view_matrix;
	notifyObservers();
}

float EC_CameraComponent::getDrawDistance()
{
	return mDrawDistance;
}

float EC_CameraComponent::getFOVAngle()
{
	return mFOVAngle;
}

void EC_CameraComponent::getViewPort(int & width, int & height)
{
	width = mViewPortWidth;
	height = mViewPortHeight;
}

glm::mat4 & EC_CameraComponent::getViewMatrix()
{
	return m_ViewMatrix;
}

ComponentType EC_CameraComponent::getComponentType()
{
	return ComponentType::Camera;
}
