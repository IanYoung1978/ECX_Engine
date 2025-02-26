#pragma once
#include "Components/IComponent.h"

class EC_Skybox_Component :
	public IComponent
{
public:
	EC_Skybox_Component();
	virtual ~EC_Skybox_Component();

	// Inherited via IComponent
	unsigned int getSkyBoxTexture();
	void setSkyBoxTexture(unsigned int texID);
	float getOpacity();
	void setOpacity(float opacity);
	bool getVisible();
	void setVisible(bool visible);
	virtual ComponentType getComponentType() override;
private:
	std::shared_ptr<GameEntity> m_Owner;
	unsigned int m_SkyBoxTexture;
	float m_Opacity;
	bool m_Visibility;
};

