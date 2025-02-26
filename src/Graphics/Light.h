#pragma once
#include "Components/IComponent.h"
#include <glm\glm.hpp>
#include "LightData.h"
enum class LightType
{
	Point,
	Directional,
	SpotLight,
	Num_Lights
};


class Light :
	public IComponent
{
public:
	Light();
	virtual ~Light();
	// Inherited via IComponent
	virtual ComponentType getComponentType() override;
	void setLightData(std::shared_ptr<LightData> data);
	void setLightType(LightType type);
	void setShadowCaster(bool caster = true);
	void setDynamic();
	void setStatic();
	void setShadowState(bool state);
	bool castsShadow();
	bool dynamic();
	LightType getLightType();
	LightData& getLightData();
private:
	std::shared_ptr<LightData> m_Data;
	LightType m_LightType;
	bool m_CastsShadow;
	bool m_Static;
};

