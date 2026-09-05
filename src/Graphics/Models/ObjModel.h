#pragma once
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <string>
#include <vector>
#include <glm\glm.hpp>
#include "Terrain/EC_TerrainMeshData.h"

class ObjModel
{
public:
	ObjModel();
	bool loadModel(const std::string& filename);
	void unload();
	unsigned int getHandle();
	unsigned int getVertCount();
	void draw() const;
	void initialise(void);
	// Additive bypass for procedurally generated meshes (e.g. EC_MarchingCubesMesher
	// output): skips Assimp/loadModel entirely, uploading only position + normal + index
	// data. Must be called on the GL/main thread, same as initialise().
	void initialiseFromMeshData(const EC_TerrainMeshData& meshData);
	virtual ~ObjModel();
private:
	void loadFromScene(const aiScene *p_assimpScene);
	void loadFromMesh(const aiMesh* p_assimpMesh);

	bool loadFromFile(std::string& filename, std::vector<std::string>& data);
	void clearModelData();
	unsigned int m_NumVerts;
	struct Mesh
	{
		Mesh()
		{
			m_NumIndices = 0;
			m_BaseVertex = 0;
			m_BaseIndex = 0;
			m_MaterialIndex = 0;
		}

		int m_NumIndices;
		int m_BaseVertex;
		int m_BaseIndex;
		int m_MaterialIndex;
	};
	std::vector<Mesh> m_MeshPool;
	std::vector<glm::vec3> m_Vertices;
	std::vector<glm::vec3> m_Normals;
	std::vector<glm::vec2> m_TextureCoordinates;
	std::vector<glm::vec3> m_Tangents;
	std::vector<glm::vec3> m_Bitangents;
	std::vector<unsigned int> m_Indices;
	unsigned int m_VBOData[7];
	unsigned int m_MeshLocation;

	bool m_Loaded;
};

