#pragma once
#include "IComponent.h"
#include "Graphics/ObjModel.h"
#include "Graphics/TextureSet.h"
#include "Graphics/Shader.h"

class GraphicsData :
	public IComponent
{
public:
	GraphicsData();
	virtual ~GraphicsData();

	// Inherited via IComponent
	virtual ComponentType getComponentType() override;
	void setModel(std::shared_ptr<ObjModel> model);
	void setModelName(const std::string& fname);
	const std::string& getModelName();
	void setShaderNames(const std::string& vert, const std::string& frag);
	const std::string& getFragShaderName();
	const std::string& getVertexShaderName();
	void setTextures(std::shared_ptr<TextureSet> textures);
	void setShader(std::shared_ptr<Shader> shader);
	void setColour(glm::vec4 colour);
	std::shared_ptr<Shader> getShader();
	std::shared_ptr<ObjModel> getModel();
	std::shared_ptr<TextureSet> getTextures();
	glm::vec4 getColour();

	void Finalize();
private:
	std::shared_ptr<ObjModel> m_Model;
	std::shared_ptr<TextureSet> m_Textures;
	std::shared_ptr<GameEntity> m_Owner;
	std::shared_ptr<Shader> m_Shader;
	bool m_loaded;
	glm::vec4 m_Colour;
	std::string m_model_name;
	std::string m_fragname;
	std::string m_vertname;
};

