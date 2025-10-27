#pragma once
#include "xml\tinyxml.h"
#include <memory>
#include "GameEntity.h"
#include "Graphics/TextureManager.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/MeshManager.h"

class EntityFactory
{
public:
	EntityFactory();
	void constructEntity(TiXmlElement& descriptor);
	void constructCamera(TiXmlElement& descriptor);
	void performPostLoadActions();
	void performPostLoadActions(std::shared_ptr<GameEntity> & entity);
	std::vector<std::shared_ptr<GameEntity>>& getCameras();
	std::vector<std::shared_ptr<GameEntity>>& getEntities();
	std::vector<std::shared_ptr<GameEntity>>& getLights();
	virtual ~EntityFactory();
	static TextureManager s_TexManager;
	static ShaderManager s_ShaderManager;
	static MeshManager s_MeshManager;
private:
	static std::vector<std::shared_ptr<GameEntity>> s_Cameras;
	static std::vector<std::shared_ptr<GameEntity>> s_entities;
	static std::vector<std::shared_ptr<GameEntity>> s_lights;
};

