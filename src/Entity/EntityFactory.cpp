#include "EntityFactory.h"
#include "Graphics/TextureSet.h"
#include "Components/Spatial.h"
#include "Components/Transform.h"
#include "Graphics/Light.h"
#include "Graphics/ObjModel.h"
#include "Components/GraphicsData.h"
#include "Graphics/Shader.h"
#include "Components/EC_CameraComponent.h"
#include <sstream>
#include "Components/EC_ScriptComponent.h"
#include "Components/EC_OnCreateHandler.h"
#include "Components/EC_OnKeyDownHandler.h"
#include "Components/EC_OnKeyHeldHandler.h"
#include "Components/EC_OnKeyUpHandler.h"
#include "Components/EC_OnMouseMoveHandler.h"
#include "Graphics/ADS_TextureSet.h"
#include "Graphics/PBR_TextureSet.h"

TextureManager EntityFactory::s_TexManager;
MeshManager EntityFactory::s_MeshManager;
ShaderManager EntityFactory::s_ShaderManager;
std::vector<std::shared_ptr<GameEntity>> EntityFactory::s_Cameras;
std::vector<std::shared_ptr<GameEntity>> EntityFactory::s_entities;
std::vector<std::shared_ptr<GameEntity>> EntityFactory::s_lights;

EntityFactory::EntityFactory()
{
	s_TexManager.init();
}

std::shared_ptr<GameEntity> EntityFactory::constructEntity(TiXmlElement & descriptor)
{
	auto e = std::make_shared<GameEntity>();

	auto att = descriptor.FirstAttribute();
	bool uidset = false;
	while (att != nullptr)
	{
		if (att->NameTStr() == "uid")
		{
			e->setUID(att->IntValue());
			uidset = true;
		}
		if (att->NameTStr() == "name")
		{
			e->setName(att->Value());
		}
		att = att->Next();
	}
	if (!uidset)
	{
		e->setUID(s_entities.size());
	}
	auto elem = descriptor.FirstChildElement();
	while (elem != nullptr)
	{
		if (strcmp(elem->Value(), "Spatial") == 0)
		{
			auto spat = std::make_shared<Spatial>();
			auto child = elem->FirstChildElement();
			while (child != nullptr)
			{
				if (strcmp(child->Value(), "Position") == 0)
				{
					std::string pos = child->GetText();
					glm::vec3 position(0.0f);
					std::stringstream ss(pos);
					char delim;
					ss >> position.x;
					ss >> delim;
					ss >> position.y;
					ss >> delim;
					ss >> position.z;
					spat->setPosition(position);
				}
				else if (strcmp(child->Value(), "Velocity") == 0)
				{
					std::string vel = child->GetText();
					glm::vec3 velocity(0.0f);
					std::stringstream ss(vel);
					char delim;
					ss >> velocity.x;
					ss >> delim;
					ss >> velocity.y;
					ss >> delim;
					ss >> velocity.z;
					spat->setVelocity(velocity);
				}
				else if (strcmp(child->Value(), "Orientation") == 0)
				{
					std::string o = child->GetText();
					glm::vec3 orient(0.0f);
					std::stringstream ss(o);
					char delim;
					ss >> orient.x;
					ss >> delim;
					ss >> orient.y;
					ss >> delim;
					ss >> orient.z;
					orient.x = glm::radians(orient.x);
					orient.y = glm::radians(orient.y);
					orient.z = glm::radians(orient.z);
					spat->setOrientation(orient);
				}
				else if (strcmp(child->Value(), "AngularVelocity") == 0)
				{
					std::string a = child->GetText();
					glm::vec3 ang(0.0f);
					std::stringstream ss(a);
					char delim;
					ss >> ang.x;
					ss >> delim;
					ss >> ang.y;
					ss >> delim;
					ss >> ang.z;
					ang.x = glm::radians(ang.x);
					ang.y = glm::radians(ang.y);
					ang.z = glm::radians(ang.z);
					spat->setAngVelocity(ang);
				}
				child = child->NextSiblingElement();
			}
			e->addComponent(std::type_index(typeid(Spatial)), spat);
		}
		else if (strcmp(elem->Value(), "Camera") == 0) 
		{
			auto child = elem->FirstChildElement();
			// get Fov and draw distance
			auto cam_comp = std::make_shared<EC_CameraComponent>();
			while (child)
			{
				if (strcmp(child->Value(),"FOV") == 0)
				{
					std::string s = child->GetText();
					std::stringstream ss(s);
					float fov;
					ss >> fov;
					cam_comp->setFOVAngle(fov);
				}
				else if (strcmp(child->Value(), "DrawDistance") == 0)
				{
					std::string s = child->GetText();
					std::stringstream ss(s);
					float drawDistance;
					ss >> drawDistance;
					cam_comp->setDrawDistance(drawDistance);
				}
				child = child->NextSiblingElement();
			}

			e->addComponent(std::type_index(typeid(EC_CameraComponent)), cam_comp);
		}
		else if (strcmp(elem->Value(), "Gfx") == 0)
		{
			auto gfx = std::make_shared<GraphicsData>();
			auto child = elem->FirstChildElement();
			while (child != nullptr)
			{
				if (strcmp(child->Value(), "ADSMaterial") == 0)
				{
					auto texSet = std::make_shared<ADS_TextureSet>();
					auto child1 = child->FirstChildElement();
					while (child1 != nullptr)
					{
						if (strcmp(child1->Value(), "Diffuse") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Diffuse, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Normal") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Normal, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Specular") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Specular, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Height") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Parallax, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Emissive") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Glow, std::string(child1->GetText()));
						}
						child1 = child1->NextSiblingElement();
						gfx->setTextures(texSet);
					}
					gfx->setTextures(texSet);
				}
				else if (strcmp(child->Value(), "PBRMaterial") == 0)
				{
					auto texSet = std::make_shared<PBR_TextureSet>();
					auto child1 = child->FirstChildElement();
					while (child1 != nullptr)
					{
						if (strcmp(child1->Value(), "Albedo") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Albedo, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Normal") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Normal, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Smoothness") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Smoothness, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Height") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Parallax, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Emissive") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Glow, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "Metallic") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::Metallic, std::string(child1->GetText()));
						}
						else if (strcmp(child1->Value(), "AO") == 0)
						{
							s_TexManager.loadTexture(std::string(child1->GetText()));
							texSet->setTexture(TextureID::AO, std::string(child1->GetText()));
						}
						child1 = child1->NextSiblingElement();
						gfx->setTextures(texSet);
					}
					gfx->setTextures(texSet);
				}
				else if (strcmp(child->Value(), "Model") == 0)
				{
					s_MeshManager.loadObjModel(child->GetText());
					gfx->setModelName(child->GetText());
				}
				else if (strcmp(child->Value(), "Shader") == 0)
				{
					auto vertex = child->FirstAttribute();
					std::string v;
					if (vertex)
					{
						v = vertex->Value();
						if (v.length() > 0)
						{
							auto frag = vertex->Next();
							if (frag)
							{
								std::string f = frag->Value();
								if (f.length() > 0)
								{
									s_ShaderManager.loadShader(v, f);
									gfx->setShaderNames(v, f);
								}
							}
						}
					}
				}
				child = child->NextSiblingElement();
			}
			e->addComponent(std::type_index(typeid(GraphicsData)), gfx);
		}
		else if (strcmp(elem->Value(), "Transform") == 0)
		{
			auto t = std::make_shared<Transform>();
			auto child = elem->FirstChildElement();
			while (child)
			{
				if (strcmp(child->Value(), "Scale") == 0)
				{
					std::string s = child->GetText();
					glm::vec3 scale(0.0f);
					std::stringstream ss(s);
					char delim;
					ss >> scale.x;
					ss >> delim;
					ss >> scale.y;
					ss >> delim;
					ss >> scale.z;
					t->setScale(scale);
				}
				child = child->NextSiblingElement();
			}
			e->addComponent(std::type_index(typeid(Transform)), t);
		}
		else if (strcmp(elem->Value(), "Light") == 0)
		{
			auto light = std::make_shared<Light>();
			auto child = elem->FirstChildElement();
			glm::vec4 position(.0f);
			glm::vec4 colour(.0f);
			glm::vec4 direction(.0f);
			glm::vec4 atten(.0f);
			float intensity=1.0f;
			float cutoffAngle=45.0f;
			while (child != nullptr)
			{
				if (strcmp(child->Value(), "Type") == 0)
				{
					std::string type = child->GetText();
					if (type == "Directional")
					{
						light->setLightType(LightType::Directional);
					}
					else if (type == "Spotlight")
					{
						light->setLightType(LightType::SpotLight);
					}
					else if (type == "Point")
					{
						light->setLightType(LightType::Point);
					}
				}
				else if (strcmp(child->Value(), "Position") == 0)
				{
					std::string pos = child->GetText();
					std::stringstream ss(pos);
					char delim;
					ss >> position.x;
					ss >> delim;
					ss >> position.y;
					ss >> delim;
					ss >> position.z;
					position.w = 1.0f;
				}
				else if (strcmp(child->Value(), "Colour") == 0)
				{
					std::string col = child->GetText();
					std::stringstream ss(col);
					char delim;
					ss >> colour.x;
					ss >> delim;
					ss >> colour.y;
					ss >> delim;
					ss >> colour.z;
					colour.w = 1.0f;
				}
				else if (strcmp(child->Value(), "Attenuation") == 0)
				{
					std::string att = child->GetText();
					std::stringstream ss(att);
					char delim;
					ss >> atten.x;
					ss >> delim;
					ss >> atten.y;
					ss >> delim;
					ss >> atten.z;
					atten.w = 0.0f;
				}
				else if (strcmp(child->Value(), "Intensity") == 0)
				{
					intensity = std::stof(child->GetText());
				}
				else if (strcmp(child->Value(), "Direction") == 0)
				{
					std::string att = child->GetText();
					glm::vec3 d;
					std::stringstream ss(att);
					char delim;
					ss >> d.x;
					ss >> delim;
					ss >> d.y;
					ss >> delim;
					ss >> d.z;
					d = glm::normalize(d);
					direction.w = 0.0f;
					direction.x = d.x;
					direction.y = d.y;
					direction.z = d.z;
				}
				else if (strcmp(child->Value(), "Cutoff") == 0)
				{
					cutoffAngle = std::stof(child->GetText());
				}
				else if (strcmp(child->Value(), "CastsShadow") == 0)
				{
					if (strcmp(child->GetText(), "true") == 0)
					{
						light->setShadowCaster(true);
					}
					else
					{
						light->setShadowCaster(false);
					}
				}
				else if(strcmp(child->Value(), "Dynamic") == 0)
				{
					if (strcmp(child->GetText(), "true") == 0)
					{
						light->setDynamic();
					}
					else
					{
						light->setStatic();
					}
				}
				child = child->NextSiblingElement();
			}
			if (light->getLightType() == LightType::Directional)
			{
				auto data = std::make_shared<DirLightData>();
				data->colour = colour;
				data->direction = direction;
				data->intensity = intensity;
				light->setLightData(data);
			}
			else if (light->getLightType() == LightType::SpotLight)
			{
				auto data = std::make_shared<SpotLightData>();
				data->colour = colour;
				data->intensity = intensity;
				data->attenuation = atten;
				data->position = position;
				data->direction = direction;
				data->cutoffAngle = glm::radians(cutoffAngle/2.0f);
				light->setLightData(data);
			}
			else if (light->getLightType() == LightType::Point)
			{
				auto data = std::make_shared<LightData>();
				data->colour = colour;
				data->intensity = intensity;
				data->attenuation = atten;
				data->position = position;
				light->setLightData(data);
			}
			e->addComponent(std::type_index(typeid(Light)), light);
		}
		else if (strcmp(elem->Value(), "Scripts") == 0)
		{
			auto scripts = std::make_shared<EC_ScriptComponent>();
			auto child = elem->FirstChildElement();
			while (child != nullptr)
			{
				if (strcmp(child->Value(), "OnKeyDown") == 0)
				{
					scripts->setBehaviour(EC_BehaviourType::key_down, std::make_shared<EC_OnKeyDownHandler>(child->GetText()));
				}
				else if (strcmp(child->Value(), "OnKeyUp") == 0)
				{
					scripts->setBehaviour(EC_BehaviourType::key_up, std::make_shared<EC_OnKeyUpHandler>(child->GetText()));
				}
				else if (strcmp(child->Value(), "OnKeyHeld") == 0)
				{
					scripts->setBehaviour(EC_BehaviourType::key_held, std::make_shared<EC_OnKeyHeldHandler>(child->GetText()));
				}
				////else if (strcmp(child->Value(), "OnUpdate") == 0)
				////{
				////	scripts->setBehaviour(EC_BehaviourType::OnUpdate, std::make_shared<EC_Lua_Script>(child->GetText()));
				////}
				else if (strcmp(child->Value(), "OnCreate") == 0)
				{
					scripts->setBehaviour(EC_BehaviourType::EntityCreate, std::make_shared<EC_OnCreateHandler>(child->GetText()));
				}
				//else if (strcmp(child->Value(), "OnDeath") == 0)
				//{
				//	scripts->setBehaviour(EC_BehaviourType::OnDeath, std::make_shared<EC_Lua_Script>(child->GetText()));
				//}
				else if (strcmp(child->Value(), "OnMouseMove") == 0)
				{
					scripts->setBehaviour(EC_BehaviourType::mouse_move, std::make_shared<EC_OnMouseMoveHandler>(child->GetText()));
				}
				child = child->NextSiblingElement();
			}
			e->addComponent(std::type_index(typeid(EC_ScriptComponent)), scripts);
		}
		elem = elem->NextSiblingElement();
	}
	e->activate();
	s_entities.push_back(e);
	return e;
}

void EntityFactory::constructCamera(TiXmlElement & descriptor)
{
	auto cam = std::make_shared<GameEntity>();
	auto elem = descriptor.FirstChildElement();
	auto cam_comp = std::make_shared<EC_CameraComponent>();
	cam->addComponent(std::type_index(typeid(EC_CameraComponent)), cam_comp);
	while (elem)
	{
		if (strcmp(elem->Value(), "Spatial") == 0)
		{
			auto spat = std::make_shared<Spatial>();
			auto e = elem->FirstChildElement();
			while (e)
			{
				if (strcmp(e->Value(), "Position") == 0)
				{
					std::string pos = e->GetText();
					glm::vec3 position(0.0f);
					std::stringstream ss(pos);
					char delim;
					ss >> position.x;
					ss >> delim;
					ss >> position.y;
					ss >> delim;
					ss >> position.z;

					spat->setPosition(position);

				}
				else if (strcmp(e->Value(), "Orientation") == 0)
				{
					std::string o = e->GetText();
					glm::vec3 orient(0.0f);
					std::stringstream ss(o);
					char delim;
					ss >> orient.x;
					ss >> delim;
					ss >> orient.y;
					ss >> delim;
					ss >> orient.z;
					orient.x = glm::radians(orient.x);
					orient.y = glm::radians(orient.y);
					orient.z = glm::radians(orient.z);
					spat->setOrientation(orient);
				}

				e = e->NextSiblingElement();
			}
			cam->addComponent(std::type_index(typeid(Spatial)), spat);
		}
		else if (strcmp(elem->Value(), "FOV") == 0)
		{
			float fov = std::stof(elem->GetText());
			cam_comp->setFOVAngle(fov);
		}
		else if (strcmp(elem->Value(), "DrawDistance") == 0)
		{
			float dd = std::stof(elem->GetText());
			cam_comp->setDrawDistance(dd);
		}
		elem = elem->NextSiblingElement();
	}
	cam->activate();
	s_Cameras.push_back(cam);
}

void EntityFactory::performPostLoadActions()
{
	s_TexManager.finalizeTextures();
	s_MeshManager.finaliseModels();
	s_ShaderManager.finaliseShaders();

	for (auto e : s_entities)
	{
		auto gfx = e->getComponent<GraphicsData>();
		if (gfx != nullptr)
		{
			gfx->Finalize();
		}
	}
}

void EntityFactory::performPostLoadActions(std::shared_ptr<GameEntity>& entity)
{
	auto gfx = entity->getComponent<GraphicsData>();
	if (gfx != nullptr)
	{
		gfx->Finalize();
	}
}

std::vector<std::shared_ptr<GameEntity>>& EntityFactory::getCameras()
{
	return s_Cameras;
}

std::vector<std::shared_ptr<GameEntity>>& EntityFactory::getEntities()
{
	return s_entities;
}

std::vector<std::shared_ptr<GameEntity>>& EntityFactory::getLights()
{
	return s_lights;
}

EntityFactory::~EntityFactory()
{
}
