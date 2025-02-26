#pragma once
#include "Renderer.h"
#include "Entity/GameEntity.h"
#include "FrameBufferSet.h"
#include "LightUniformBuffer.h"
#include "ShadowBuffer.h"
#include "ShadowCubeBuffer.h"
#include "Shader.h"
#include "ShadowData.h"
#include "Common/EC_ProxyObserver.h"
#include "EC_Skybox_Component.h"
#include "Components/EC_CameraComponent.h"
#include "Components/GraphicsData.h"
#include "Components/Transform.h"
#include "Components/Spatial.h"

class GFX_Proxy :
	public EC_ProxyObserver
{
public:
	GFX_Proxy()
	{
		hasTextures = false;
		id = 0;
		mesh_ID = 0;
		num_Verts = 0;
	}
	GFX_Proxy(std::shared_ptr<GameEntity>& e);
	GFX_Proxy(GFX_Proxy&& proxy) noexcept;
	GFX_Proxy(GFX_Proxy& proxy);
	GFX_Proxy& operator=(GFX_Proxy& proxy);
	~GFX_Proxy();
	void Register();
	void UnRegister();
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;
	unsigned int num_Verts;
	unsigned int mesh_ID;
	bool hasTextures;
	glm::mat4 transform;
	std::shared_ptr<Shader> shader;
	std::shared_ptr<GameEntity> entity;
	std::shared_ptr<GraphicsData> gfx_data;
	std::shared_ptr<Transform> trans;
	std::shared_ptr<TextureSet> textures;
	glm::vec4 colour;
};

class EC_CameraProxy :
	public EC_ProxyObserver
{
public:
	EC_CameraProxy();
	EC_CameraProxy(std::shared_ptr<GameEntity>& e);
	EC_CameraProxy(EC_CameraProxy&& proxy) noexcept;
	EC_CameraProxy& operator=(EC_CameraProxy& proxy);
	~EC_CameraProxy();
	void Register();
	void UnRegister();
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;
	std::shared_ptr<EC_CameraComponent> cam;
	std::shared_ptr<GameEntity> entity;
	std::shared_ptr<Spatial> spatial;
	glm::mat4 view_matrix;
};

class SkyBox_Proxy:
	public EC_ProxyObserver
{
public:
	SkyBox_Proxy();
	SkyBox_Proxy(std::shared_ptr<GameEntity>& e);
	~SkyBox_Proxy() noexcept;
	SkyBox_Proxy(SkyBox_Proxy&& proxy) noexcept;
	SkyBox_Proxy(SkyBox_Proxy& proxy) noexcept;
	SkyBox_Proxy& operator=(SkyBox_Proxy& proxy);
	void Register();
	void UnRegister();
	bool visible;
	float opacity;
	unsigned int texture;
	std::shared_ptr<EC_Skybox_Component> skybox;
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;
};

class Light_Proxy :
	public EC_ProxyObserver
{
public:
	Light_Proxy();
	Light_Proxy(std::shared_ptr<GameEntity>& e);
	~Light_Proxy();
	Light_Proxy(Light_Proxy&& proxy) noexcept;
	Light_Proxy(Light_Proxy& proxy) noexcept;
	Light_Proxy & operator =(Light_Proxy& proxy);
	void Register();
	void UnRegister();
	// Inherited via EC_Observer
	virtual void notify() override;
	virtual bool operator==(EC_Observer & other) override;
	bool m_CastsShadow;
	bool m_Static;
	std::shared_ptr<GameEntity> entity;
	std::shared_ptr<Light> light;
};
enum class PostProcess
{
	MSAA,
	HDR,
	DOF,
	NumProcesses
};
class GL_Deferred_Renderer :
	public Renderer
{
public:
	GL_Deferred_Renderer();
	virtual ~GL_Deferred_Renderer();
	// Inherited via Renderer
	virtual void init(std::shared_ptr<Window> window) override;
	virtual void addEntity(std::shared_ptr<GameEntity> e) override;
	virtual void removeEntity(unsigned int entityID) override;
	virtual void renderScene() override;
	virtual void changeResolution(int width, int height) override;
	virtual void clearScene() override;
private:
	void createProxy(std::shared_ptr<GameEntity> e);
	void removeProxy(unsigned int entityID);
	void geometryPass();
	void lightPass();
	void postProcess();
	void glowPass();
	void renderQuad();
	void finalPass();
	void updateLights();
	void shadowDirPass(ShadowBuffer& target, DirLightData& light);
	void shadowSpotPass(ShadowBuffer& target, SpotLightData& light);
	void shadowPointPass(ShadowBuffer& target, LightData& light);
	void shadowLightingPass();


	std::mutex m_lock;
	LightUniformBuffer m_LightBuffer;
	FrameBufferSet m_FrameBuffer;
	std::shared_ptr<Window> m_Window;
	std::vector<std::shared_ptr<GameEntity>> m_toAdd;
	std::vector<unsigned int> m_toRemove;
	std::vector<GFX_Proxy> m_Proxies;
	std::vector<Light_Proxy> m_lights;
	std::vector<SkyBox_Proxy> m_Skyboxes;
	std::vector<EC_CameraProxy> m_cameras;
	size_t m_ActiveCamera;
	std::shared_ptr<Shader> m_LightPassShader;
	std::vector<std::shared_ptr<Shader>> m_PostProcessShaders;
	unsigned int m_FS_QuadHandle;
	unsigned int m_FS_QuadVerts;
	unsigned int m_FS_QuadTex;
	unsigned int m_FS_QuadIndices;
	bool m_HDR;
	bool m_DOF;
	bool m_MSAA;
	std::vector<LightData> m_Points;
	std::vector<SpotLightData> m_Spots;
	std::vector<DirLightData> m_Directionals;
	Shader m_ShadowShader;
	Shader m_ShadowDirLightShader;
	Shader m_ShadowSpotLightShader;
	Shader m_ShadowPointLightShader;
	Shader m_BloomVShader;
	Shader m_BloomHShader;
	ShadowBuffer m_ShadowBuffer;
	glm::mat4 m_ShadowDirMatrix;
	glm::mat4 m_ShadowSpotMatrix;
	ShadowData m_ShadowPointMatrices;
	ShadowCubeBuffer m_ShadowCubeBuffer;
	std::vector<DirLightData> m_ShadowDirs;
	std::vector<LightData> m_ShadowPoints;
	std::vector<SpotLightData> m_ShadowSpots;
};

