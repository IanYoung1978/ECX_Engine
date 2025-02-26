#pragma once
#include "IComponent.h"
#include <glm\glm.hpp>

class EC_CameraComponent :
	public IComponent
{
public:
	EC_CameraComponent();
	virtual ~EC_CameraComponent();
	void setDrawDistance(float drawDistance);
	void setFOVAngle(float fov_radians);
	void setViewPort(int width, int height);
	void setActive(bool toggle);
	void setViewMatrix(glm::mat4& view_matrix);
	float getDrawDistance();
	float getFOVAngle();
	void getViewPort(int& width, int& height);
	glm::mat4& getViewMatrix();
	virtual ComponentType getComponentType() override;
private:
	float mDrawDistance;
	float mFOVAngle;
	int mViewPortWidth;
	int mViewPortHeight;
	bool m_IsActive;
	glm::mat4 m_ViewMatrix;
};

