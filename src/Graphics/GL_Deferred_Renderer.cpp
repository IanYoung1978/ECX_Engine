#include "GL_Deferred_Renderer.h"
#include "Components/GraphicsData.h"
#include "Components/Transform.h"
#include "TextureSet.h"
#include "ObjModel.h"
#include "Light.h"
#include "BufferType.h"
#include <glm\gtc\matrix_transform.hpp>
#include "Entity/EntityFactory.h"
#include "Components/ProxyHelper.h"
#include "Logging/ECX_Logging.h"
#include "Window/Window.h"
#include "Entity/EntityManager.h"

GL_Deferred_Renderer::GL_Deferred_Renderer()
{
	m_MSAA = false;
	m_DOF = false;
	m_HDR = false;
	m_ActiveCamera = 0;
	m_FS_QuadHandle = 0;
	m_FS_QuadIndices = 0;
	m_FS_QuadTex = 0;
	m_FS_QuadVerts = 0;
}

GL_Deferred_Renderer::~GL_Deferred_Renderer()
{

}

void GL_Deferred_Renderer::init(std::shared_ptr<Window> window)
{
	m_Window = window;

	//create full screen quad

	glm::vec3 verts[] = { 
		glm::vec3(-1.0f,-1.0f,0.0f),	//0
		glm::vec3( 1.0f,-1.0f,0.0f),	//1
		glm::vec3( 1.0f, 1.0f,0.0f),	//2
		glm::vec3(-1.0f, 1.0f,0.0f) };	//3

	glm::vec2 texCoords[] = { 
		glm::vec2(0.0f,0.0f),
		glm::vec2(1.0f,0.0f),
		glm::vec2(1.0f,1.0f),
		glm::vec2(0.0f,1.0f),};

	unsigned int indices[] = { 0,1,2,0,2,3 };
	//////////////////////////////////////////////////////////////////////
	//				uv(0,0)								uv(1,0)										
	//					-------------------------------------
	//			v(-1,1)	| i(3)						   i(2) |	v(1,1)
	//					|									|
	//					|									|
	//					|									|
	//			v(-1,-1)| i(0)						   i(1) |	v(1,-1)
	//					-------------------------------------
	//				uv(0,1)								uv(1,1)
	///////////////////////////////////////////////////////////////////////

	glGenVertexArrays(1, &m_FS_QuadHandle);
	glBindVertexArray(m_FS_QuadHandle);
	glGenBuffers(1, &m_FS_QuadVerts);
	glBindBuffer(GL_ARRAY_BUFFER, m_FS_QuadVerts);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 4, verts, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)BufferType::Vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray((GLuint)BufferType::Vertex);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &m_FS_QuadTex);
	glBindBuffer(GL_ARRAY_BUFFER, m_FS_QuadTex);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 4, texCoords, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)BufferType::TextureCoordinate, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray((GLuint)BufferType::TextureCoordinate);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenBuffers(1, &m_FS_QuadIndices);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_FS_QuadIndices);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	//create light and final pass shaders
	m_LightPassShader = std::make_shared<Shader>();

	// TODO: Get rid of these hard coded paths and implement a config file
	// TODO: Update API to 1, Take a config file path and load it

	if (!m_LightPassShader->loadShader(std::string("data/assets/shaders/lightpass.vert"), std::string("data/assets/shaders/lightpass.frag")))  // <<-- config controlled
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load light pass shader", LOGGING::LogLevel::CRITICAL);
	}

	if (!m_ShadowShader.loadShader(std::string("data/assets/shaders/shadow.vert"), std::string("data/assets/shaders/shadow.frag")))  // <<-- config controlled
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load point shadow pass shader", LOGGING::LogLevel::CRITICAL);
	}
	
	if (!m_ShadowDirLightShader.loadShader(std::string("data/assets/shaders/ShadowLightPass.vert"), std::string("data/assets/shaders/DirLightShadowPBR.frag")))  // <<-- config controlled
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load directional shadow pass shader", LOGGING::LogLevel::CRITICAL);
	}

	if (!m_ShadowSpotLightShader.loadShader(std::string("data/assets/shaders/ShadowLightPass.vert"), std::string("data/assets/shaders/SpotlightShadowPBR.frag")))  // <<-- config controlled
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load spot shadow pass shader", LOGGING::LogLevel::CRITICAL);
	}
	
	if (!m_BloomHShader.loadShader(std::string("data/assets/shaders/final.vert"), std::string("data/assets/shaders/bloomH.frag"))
		||
		!m_BloomVShader.loadShader(std::string("data/assets/shaders/final.vert"), std::string("data/assets/shaders/bloomV.frag")))  // <<-- config controlled
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("failed to load bloom shader", LOGGING::LogLevel::CRITICAL);
	}

	m_FrameBuffer.init(window->getWidth(), window->getHeight());
	m_LightBuffer.init((*m_LightPassShader));
	m_ShadowBuffer.init(2048, 2048);  // <<-- config controlled
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void GL_Deferred_Renderer::renderScene()
{
	m_FrameBuffer.initFrame();
	geometryPass();
	glowPass();
	lightPass();

	//postProcess();
	finalPass();
}

void GL_Deferred_Renderer::changeResolution(int width, int height)
{
	m_FrameBuffer.resize(width, height);
}

//void GL_Deferred_Renderer::clearScene()
//{
//	m_cameras.clear();
//	m_Proxies.clear();
//	m_lights.clear();
//	m_Skyboxes.clear();
//	m_Points.clear();
//	m_Spots.clear();
//	m_Directionals.clear();
//	m_ShadowDirs.clear();
//	m_ShadowPoints.clear();
//	m_ShadowSpots.clear();
//}


void GL_Deferred_Renderer::geometryPass()
{
	auto cameras = EntityManager::getInstance().getEntitiesWithComponent(std::type_index(typeid(EC_CameraComponent)));
	for (size_t j = 0; j < cameras.size(); j++)
	{
		
		if (cameras[j]->isActive())
		{
			auto cam = cameras[j]->getComponent<EC_CameraComponent>();
			auto cam_spatial = cameras[j]->getComponent<Spatial>();
			glm::mat4 view(1.0f);
			glm::mat4 projection(1.0f);

			projection = glm::perspective(
				glm::radians(cam->getFOVAngle()),
				(float)m_Window->getWidth() / m_Window->getHeight(),
				1.0f,
				cam->getDrawDistance()
			);

			view = cam->getViewMatrix();

			m_FrameBuffer.GeometryPass();

			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			auto entities = EntityManager::getInstance().getEntitiesWithComponents({
				std::type_index(typeid(Transform)),
				std::type_index(typeid(GraphicsData))
				});

			for (size_t i = 0; i < entities.size(); i++)
			{
				if (entities[i]->isActive())
				{
					auto transform = entities[i]->getComponent	<Transform>()->getTransform();
					auto gfx = entities[i]->getComponent<GraphicsData>();
					gfx->Finalize();
					auto shader = gfx->getShader();
					glm::mat3 nm = glm::transpose(glm::inverse(transform));
					shader->activate();
					
					shader->setUniform("camPos", cam_spatial->getPosition());
					//entities[i].shader->setUniform("NormalTransform", nm);
					shader->setUniform("ViewTransform", view);
					shader->setUniform("ProjTransform", projection);
					shader->setUniform("ModelTransform", transform);

					if (gfx->getTextures()!= nullptr)
					{
						shader->setUniform("hasMaterial", 1);
						gfx->getTextures()->bindTextures(shader);
					}
					else
					{
						shader->setUniform("hasMaterial", 0);
						shader->setUniform("incolour", gfx->getColour());
					}
					glBindVertexArray(gfx->getModel()->getHandle());
					glDrawElements(GL_TRIANGLES, gfx->getModel()->getVertCount(), GL_UNSIGNED_INT, 0);
					glBindVertexArray(0);
					for (int i = 0; i < 10; i++)
					{
						glActiveTexture(GL_TEXTURE0 + i);
						glBindTexture(GL_TEXTURE_2D, 0);
					}
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glUseProgram(0);
			glBindTexture(GL_TEXTURE_2D, 0);
			break;
		}
	}
}

void GL_Deferred_Renderer::lightPass()
{
	if (m_LightPassShader)
	{
		auto cameras = EntityManager::getInstance().getEntitiesWithComponent(std::type_index(typeid(EC_CameraComponent)));
		for (auto& cam_entity : cameras)
		{
			if (cam_entity->isActive())
			{
				auto cam = cam_entity->getComponent<EC_CameraComponent>();
				auto spatial = cam_entity->getComponent<Spatial>();
				updateLights();
				// activate shader for non shadow casting lights
				m_LightPassShader->activate();
				m_FrameBuffer.LightingPass(*m_LightPassShader);
				m_LightBuffer.updatePointLights((int)m_Points.size(), m_Points.data());
				m_LightBuffer.updateSpotLights((int)m_Spots.size(), m_Spots.data());

				m_LightBuffer.bindPointLights();
				m_LightPassShader->setUniform(("NumPoints"), (int)m_Points.size());
				m_LightBuffer.bindSpotLights();
				m_LightPassShader->setUniform(("NumSpots"), (int)m_Spots.size());
				m_LightPassShader->setUniform(("WSCamPos"), spatial->getPosition());
				if (!m_Directionals.empty())
					m_LightPassShader->setDirLight(("dirLight"), m_Directionals[0]);
				//render
				renderQuad();

				shadowLightingPass();
				break;
			}
		}
	}
}

void GL_Deferred_Renderer::postProcess()
{
	if (!m_PostProcessShaders.empty())
	{
		if (m_MSAA)
		{
			m_PostProcessShaders[(size_t)PostProcess::MSAA]->activate();
			m_FrameBuffer.PostProcessPass();
			renderQuad();
		}
		if (m_HDR)
		{
			m_PostProcessShaders[(size_t)PostProcess::HDR]->activate();
			m_FrameBuffer.PostProcessPass();
			renderQuad();
		}
		if (m_DOF)
		{
			m_PostProcessShaders[(size_t)PostProcess::DOF]->activate();
			m_FrameBuffer.PostProcessPass();
			renderQuad();
		}
	}
}

void GL_Deferred_Renderer::glowPass()
{
	// Draw from glow buffer to framebuffer1
	// sample from framebuffer1 to framebuffer2 (H)
	// sample fromm framebuffer2 to framebuffer1 (V)
	// repeat x54
	m_BloomHShader.activate();
	m_FrameBuffer.GlowPass(m_BloomHShader, true, false);
	renderQuad();
	for (int i = 0; i < 5; i++)
	{
		m_BloomHShader.activate();
		m_FrameBuffer.GlowPass(m_BloomHShader, false, false);
		renderQuad();
		m_BloomVShader.activate();
		m_FrameBuffer.GlowPass(m_BloomVShader, false, false);
		renderQuad();
	}
	m_FrameBuffer.GlowPass(m_BloomHShader, false, true);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void GL_Deferred_Renderer::finalPass()
{
	m_FrameBuffer.FinalPass();
}

void GL_Deferred_Renderer::updateLights()
{
	m_Directionals.clear();
	m_Spots.clear();
	m_Points.clear();
	m_ShadowDirs.clear();
	m_ShadowSpots.clear();
	m_ShadowPoints.clear();


	auto lights = EntityManager::getInstance().getEntitiesWithComponent(std::type_index(typeid(Light)));

	//process directional and spotlights
	for (size_t i = 0; i < lights.size() ; i++)
	{
		if (lights[i]->isActive())
		{
			auto light = lights[i]->getComponent<Light>();
			if (light->getLightType() == LightType::Directional)
			{
				if (!light->castsShadow())
					m_Directionals.push_back(static_cast<DirLightData&>(light->getLightData()));
				else
					m_ShadowDirs.push_back(static_cast<DirLightData&>(light->getLightData()));

			}
			else if (light->getLightType() == LightType::SpotLight)
			{
				if (!light->castsShadow())
					m_Spots.push_back(static_cast<SpotLightData&>(light->getLightData()));
				else
					m_ShadowSpots.push_back(static_cast<SpotLightData&>(light->getLightData()));
			}
			else
			{
				if (!light->castsShadow())
					m_Points.push_back(light->getLightData());
				else
					m_ShadowPoints.push_back(light->getLightData());
			}
		}
	}
}

void GL_Deferred_Renderer::shadowDirPass(ShadowBuffer & target, DirLightData& light)
{
	glm::mat4 biasMatrix(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f);

	glm::mat4 view(1.0f);
	glm::mat4 projection(1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0, 2.0);
	glCullFace(GL_FRONT);
	target.bind();
	glClearDepth(1.0);

	glm::vec3 eye = glm::vec3(-light.direction);
	glm::vec3 up;
	up[0] = eye[1] - eye[2];
	up[1] = eye[2] - eye[0];
	up[2] = eye[0] - eye[1];
	view = glm::lookAt(eye, glm::vec3(0.0f), up); // This breaks when a light has direction (0.0,-1.0,0.0): gimbal lock scenario
	projection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -10.0f, 20.0f);
	glm::mat4 pv = projection*view;
	m_ShadowDirMatrix = biasMatrix*pv;
	auto entities = EntityManager::getInstance().getEntitiesWithComponents({
		std::type_index(typeid(Transform)),
		std::type_index(typeid(GraphicsData))
		});
	for (auto const &e : entities)
	{
		if (e->isActive())
		{
			auto transform = e->getComponent<Transform>()->getTransform();
			auto gfx = e->getComponent<GraphicsData>();
			auto model = gfx->getModel();
			if (model != nullptr)
			{
				//activate shadow shader
				m_ShadowShader.activate();
				//set transforms
				m_ShadowShader.setUniform(std::string("ViewTransform"), view);
				m_ShadowShader.setUniform(std::string("ProjTransform"), projection);
				m_ShadowShader.setUniform(std::string("ModelTransform"), transform);

				glBindVertexArray(model->getHandle());
				glDrawElements(GL_TRIANGLES, model->getVertCount(), GL_UNSIGNED_INT, 0);
				glBindVertexArray(0);
			}
		}
	}
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glCullFace(GL_BACK);
}

void GL_Deferred_Renderer::shadowSpotPass(ShadowBuffer & target, SpotLightData& light)
{
	glm::mat4 biasMatrix(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.5f, 0.0f,
		0.5f, 0.5f, 0.5f, 1.0f);

	glm::mat4 view(1.0f);
	glm::mat4 projection(1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0, 2.0);
	glCullFace(GL_FRONT);
	target.bind();
	glClearDepth(1.0);

	glm::vec3 position = light.position;
	glm::vec3 direction = light.direction;
	float fov = light.cutoffAngle;
	//set camera to have same properties of light
	glm::vec3 up;
	up[0] = direction[1] - direction[2];
	up[1] = direction[2] - direction[0];
	up[2] = direction[0] - direction[1];
	view = glm::lookAt(position, position + direction, up);
	projection = glm::perspective(45.0f, 1.0f, 1.0f, 100.0f);
	glm::mat4 pv = projection*view;
	m_ShadowSpotMatrix = biasMatrix*pv;

	auto entities = EntityManager::getInstance().getEntitiesWithComponents({
		std::type_index(typeid(Transform)),
		std::type_index(typeid(GraphicsData))
		});
	for (auto const& e : entities)
	{
		if (e->isActive())
		{
			auto transform = e->getComponent<Transform>()->getTransform();
			auto gfx = e->getComponent<GraphicsData>();
			auto model = gfx->getModel();
			if (model == nullptr)
				continue;
			//activate shadow shader
			m_ShadowShader.activate();
			//set transforms
			m_ShadowShader.setUniform(std::string("ViewTransform"), view);
			m_ShadowShader.setUniform(std::string("ProjTransform"), projection);
			m_ShadowShader.setUniform(std::string("ModelTransform"), transform);

			//draw
			glBindVertexArray(gfx->getModel()->getHandle());
			glDrawElements(GL_TRIANGLES, gfx->getModel()->getVertCount(), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}
	}
	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_CULL_FACE);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glCullFace(GL_BACK);
}

void GL_Deferred_Renderer::shadowPointPass(ShadowBuffer & target, LightData& light)
{

}

void GL_Deferred_Renderer::shadowLightingPass()
{
	auto cameras = EntityManager::getInstance().getEntitiesWithComponent(std::type_index(typeid(EC_CameraComponent)));
	for (auto& cam_entity : cameras)
	{
		if (cam_entity->isActive())
		{
			auto cam = cam_entity->getComponent<EC_CameraComponent>();
			auto spatial = cam_entity->getComponent<Spatial>();
			for (size_t i = 0; i < m_ShadowDirs.size(); i++)
			{
				//render shadow map
				shadowDirPass(m_ShadowBuffer, m_ShadowDirs[i]);

				// activate shader for non shadow casting lights
				m_ShadowDirLightShader.activate();
				m_FrameBuffer.LightingPass(m_ShadowDirLightShader);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				m_ShadowDirLightShader.setUniform(("WSCamPos"), spatial->getPosition());
				m_ShadowDirLightShader.setDirLight(("dirLight"), static_cast<DirLightData&>(m_ShadowDirs[i]));
				m_ShadowDirLightShader.setUniform(("ShadowTransform"), m_ShadowDirMatrix);
				//bind depth map
				m_ShadowDirLightShader.bindTexture(("shadowMap"), 5, m_ShadowBuffer.getDepthTexture());
				//render
				renderQuad();
				glDisable(GL_BLEND);
			}
			for (size_t i = 0; i < m_ShadowSpots.size(); i++)
			{
				shadowSpotPass(m_ShadowBuffer, m_ShadowSpots[i]);
				m_ShadowSpotLightShader.activate();
				m_FrameBuffer.LightingPass(m_ShadowSpotLightShader);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				m_ShadowSpotLightShader.setUniform(("WSCamPos"), spatial->getPosition());
				m_ShadowSpotLightShader.setSpotLight(("spotLight"), static_cast<SpotLightData&>(m_ShadowSpots[i]));
				m_ShadowDirLightShader.setUniform(("ShadowTransform"), m_ShadowSpotMatrix);
				//bind depth map
				m_ShadowSpotLightShader.bindTexture(("shadowMap"), 5, m_ShadowBuffer.getDepthTexture());
				//render
				renderQuad();
				glDisable(GL_BLEND);
			}
			for (size_t i = 0; i < m_ShadowPoints.size(); i++)
			{
				shadowPointPass(m_ShadowBuffer, m_ShadowPoints[i]);
				m_ShadowPointLightShader.activate();
				m_FrameBuffer.LightingPass(m_ShadowSpotLightShader);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				m_ShadowPointLightShader.setUniform(("WSCamPos"), spatial->getPosition());
				m_ShadowPointLightShader.setLight(("pointLight"), m_ShadowPoints[i]);
				m_ShadowPointLightShader.setUniform(("ShadowTransform"), m_ShadowDirMatrix);
				//bind depth map
				m_ShadowPointLightShader.bindTexture(("shadowMap"), 5, m_ShadowBuffer.getDepthTexture());
				//render
				renderQuad();
				glDisable(GL_BLEND);
			}
			break;
		}
	}


	glUseProgram(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GL_Deferred_Renderer::renderQuad()
{
	glBindVertexArray(m_FS_QuadHandle);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

GFX_Proxy::GFX_Proxy(std::shared_ptr<GameEntity>& e)
{
	hasTextures = false;
	EntityFactory f;
	entity = e;
	id = ProxyHelper::getUID();
	
	f.performPostLoadActions(e);
	gfx_data = e->getComponent<GraphicsData>();
	shader = gfx_data->getShader();
	mesh_ID = gfx_data->getModel()->getHandle();
	num_Verts = gfx_data->getModel()->getVertCount();
	trans = e->getComponent<Transform>();
	transform = trans->getTransform();
	//textures
	textures = gfx_data->getTextures();
	if (textures != nullptr)
	{
		hasTextures = true;
	}
	colour = gfx_data->getColour();
	trans->Register(this);
	gfx_data->Register(this);
}

GFX_Proxy::GFX_Proxy(GFX_Proxy&& proxy) noexcept
{
	hasTextures = false;
	entity = proxy.entity;
	id = proxy.id;
	gfx_data = proxy.gfx_data;
	EntityFactory f;
	f.performPostLoadActions(proxy.entity);
	shader = gfx_data->getShader();
	mesh_ID = gfx_data->getModel()->getHandle();
	num_Verts = gfx_data->getModel()->getVertCount();
	trans = proxy.trans;
	transform = trans->getTransform();
	//textures
	textures = gfx_data->getTextures();
	if (textures != nullptr)
	{
		hasTextures = true;
	}
	colour = gfx_data->getColour();
	proxy.UnRegister();
	trans->Register(this);
	gfx_data->Register(this);
}

GFX_Proxy::GFX_Proxy(GFX_Proxy & proxy)
{
	hasTextures = false;
	entity = proxy.entity;
	id = proxy.id;
	gfx_data = proxy.gfx_data;
	EntityFactory f;
	f.performPostLoadActions(proxy.entity);
	shader = gfx_data->getShader();
	mesh_ID = gfx_data->getModel()->getHandle();
	num_Verts = gfx_data->getModel()->getVertCount();
	trans = proxy.trans;
	transform = trans->getTransform();
	//textures
	textures = gfx_data->getTextures();
	if (textures != nullptr)
	{
		hasTextures = true;
	}
	colour = gfx_data->getColour();
	trans->UnRegister(this);
	gfx_data->UnRegister(this);
	trans->Register(this);
	gfx_data->Register(this);
}

GFX_Proxy & GFX_Proxy::operator=(GFX_Proxy & proxy)
{
	entity = proxy.entity;
	id = proxy.id;
	gfx_data = proxy.gfx_data;
	shader = gfx_data->getShader();
	mesh_ID = gfx_data->getModel()->getHandle();
	num_Verts = gfx_data->getModel()->getVertCount();
	trans = proxy.trans;
	transform = trans->getTransform();
	//textures
	textures = gfx_data->getTextures();
	if (textures != nullptr)
	{
		hasTextures = true;
	}
	colour = gfx_data->getColour();

	proxy.UnRegister();
	Register();
	return *this;
}

GFX_Proxy::~GFX_Proxy()
{

}

void GFX_Proxy::Register()
{
	trans->Register(this);
	gfx_data->Register(this);
}

void GFX_Proxy::UnRegister()
{
	trans->UnRegister(this);
	gfx_data->UnRegister(this);
}

void GFX_Proxy::notify()
{

	auto texSet = gfx_data->getTextures();
	auto model = gfx_data->getModel();
	textures = gfx_data->getTextures();
	if (textures != nullptr)
	{
		hasTextures = true;
	}
	colour = gfx_data->getColour();
	num_Verts = model->getVertCount();
	mesh_ID = model->getHandle();
	transform = trans->getTransform();
	shader = gfx_data->getShader();
}

bool GFX_Proxy::operator==(EC_Observer & other)
{
	auto o = dynamic_cast<GFX_Proxy*>(&other);
	if (!o)
		return false;
	if (o->id == id)
		return true;
	return false;
}

Light_Proxy::Light_Proxy():
	m_CastsShadow(false),
	m_Static(true)
{
}

Light_Proxy::Light_Proxy(std::shared_ptr<GameEntity>& e) :
	m_CastsShadow(false),
	m_Static(true)
{
	id = ProxyHelper::getUID();
	entity = e;
	light = e->getComponent<Light>();
	// get other light data params necessary for lighting calculations
	light->UnRegister(this);
	light->Register(this);
	m_CastsShadow = light->castsShadow();
	m_Static = !light->dynamic();
}

Light_Proxy::~Light_Proxy()
{

}

Light_Proxy::Light_Proxy(Light_Proxy && proxy) noexcept :
	m_CastsShadow(false),
	m_Static(true) 
{
	id = proxy.id;
	entity = proxy.entity;
	light = proxy.light;
	// get other light data params necessary for lighting calculations
	light->UnRegister(this);
	light->Register(this);
	m_CastsShadow = light->castsShadow();
	m_Static = !light->dynamic();
}

Light_Proxy::Light_Proxy(Light_Proxy & proxy) noexcept:
	m_CastsShadow(false),
	m_Static(true)
{
	id = proxy.id;
	entity = proxy.entity;
	light = proxy.light;
	// get other light data params necessary for lighting calculations

	m_CastsShadow = light->castsShadow();
	m_Static = !light->dynamic();
	Register();
}

Light_Proxy & Light_Proxy::operator=(Light_Proxy & proxy)
{
	entity = proxy.entity;
	light = proxy.light;
	// get other light data params necessary for lighting calculations
	proxy.UnRegister();
	light->Register(this);
	m_CastsShadow = light->castsShadow();
	m_Static = !light->dynamic();
	return *this;
}

void Light_Proxy::Register()
{
	light->Register(this);
}

void Light_Proxy::UnRegister()
{
	light->UnRegister(this);
}

void Light_Proxy::notify()
{
	//update lighting params
	m_CastsShadow = light->castsShadow();
	m_Static = !light->dynamic();
}

bool Light_Proxy::operator==(EC_Observer & other)
{
	auto proxy = dynamic_cast<Light_Proxy*>(&other);
	if (!proxy)
		return false;
	if (id == proxy->id)
		return true;
	return false;
}

SkyBox_Proxy::SkyBox_Proxy() :
	opacity(0),
	texture(0),
	visible(true)
{
	id = 0;
}

SkyBox_Proxy::SkyBox_Proxy(std::shared_ptr<GameEntity>& e) :
	opacity(0),
	texture(0),
	visible(true)
{
	id = ProxyHelper::getUID();
	skybox = e->getComponent<EC_Skybox_Component>();
	visible = skybox->getVisible();
	opacity = skybox->getOpacity();
	texture = skybox->getSkyBoxTexture();
	Register();
}

SkyBox_Proxy::~SkyBox_Proxy() noexcept
{
}

SkyBox_Proxy::SkyBox_Proxy(SkyBox_Proxy && proxy) noexcept:
	opacity(0),
	texture(0),
	visible(true)
{
	id = proxy.id;
	skybox = proxy.skybox;
	visible = skybox->getVisible();
	opacity = skybox->getOpacity();
	texture = skybox->getSkyBoxTexture();
	proxy.UnRegister();
	Register();
}

SkyBox_Proxy::SkyBox_Proxy(SkyBox_Proxy & proxy) noexcept :
	opacity(0),
	texture(0),
	visible(true)
{
	id = proxy.id;
	skybox = proxy.skybox;
	visible = skybox->getVisible();
	opacity = skybox->getOpacity();
	texture = skybox->getSkyBoxTexture();
	proxy.UnRegister();
	Register();
}

SkyBox_Proxy & SkyBox_Proxy::operator=(SkyBox_Proxy & proxy)
{
	id = proxy.id;
	skybox = proxy.skybox;
	visible = skybox->getVisible();
	opacity = skybox->getOpacity();
	texture = skybox->getSkyBoxTexture();
	proxy.UnRegister();
	Register();
	return *this;
}

void SkyBox_Proxy::Register()
{
	skybox->Register(this);
}

void SkyBox_Proxy::UnRegister()
{
	skybox->UnRegister(this);
}

void SkyBox_Proxy::notify()
{
	visible = skybox->getVisible();
	opacity = skybox->getOpacity();
	texture = skybox->getSkyBoxTexture();
}

bool SkyBox_Proxy::operator==(EC_Observer & other)
{
	auto proxy = dynamic_cast<SkyBox_Proxy*>(&other);
	if (!proxy)
		return false;
	if (id == proxy->id)
		return true;
	return false;
}

EC_CameraProxy::EC_CameraProxy()
{
}

EC_CameraProxy::EC_CameraProxy(std::shared_ptr<GameEntity>& e)
{
	entity = e;
	// get proxy data from entity
	cam = e->getComponent < EC_CameraComponent>();
	spatial = e->getComponent<Spatial>();
	view_matrix = cam->getViewMatrix();
	cam->Register(this);
	id = ProxyHelper::getUID();
}

EC_CameraProxy::EC_CameraProxy(EC_CameraProxy && proxy) noexcept
{
	entity = proxy.entity;
	// get proxy data from entity
	cam = proxy.cam;
	spatial = proxy.spatial;
	view_matrix = cam->getViewMatrix();
	proxy.UnRegister();
	Register();
	id = proxy.id;
}

EC_CameraProxy & EC_CameraProxy::operator=(EC_CameraProxy & proxy)
{
	entity = proxy.entity;
	// get proxy data from entity
	cam = proxy.cam;
	spatial = proxy.spatial;
	view_matrix = cam->getViewMatrix();
	proxy.UnRegister();
	Register();
	id = proxy.id;
	return *this;
}

EC_CameraProxy::~EC_CameraProxy()
{
}

void EC_CameraProxy::Register()
{
	cam->Register(this);
}

void EC_CameraProxy::UnRegister()
{
	cam->UnRegister(this);
}

void EC_CameraProxy::notify()
{
	view_matrix = cam->getViewMatrix();
}

bool EC_CameraProxy::operator==(EC_Observer & other)
{
	auto proxy = dynamic_cast<EC_CameraProxy*>(&other);
	if (!proxy)
		return false;
	if (id == proxy->id)
		return true;
	return false;
}
