#include "Graphics/Models/ObjModel.h"
#include <glm\glm.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include "Graphics/FrameBuffers/BufferType.h"
#include <GL\glew.h>

struct face
{
	unsigned int verts[3];
	unsigned int normals[3];
	unsigned int texcoords[3];
};
ObjModel::ObjModel()
{
	m_MeshLocation = 0;
	m_NumVerts = 0;
	m_VBOData[0] = 0;
	m_VBOData[1] = 0;
	m_VBOData[2] = 0;
	m_VBOData[3] = 0;
	m_VBOData[4] = 0;
	m_VBOData[5] = 0;
	m_VBOData[6] = 0;
	m_Loaded = false;
}

bool ObjModel::loadModel(const std::string& filename)
{
	Assimp::Importer v_assimpImporter;	// Handle for importing the model

	const aiScene* v_assimpScene = v_assimpImporter.ReadFile(filename,
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals);

	if (!v_assimpScene)	// Check if loading was successful
		return false;

	loadFromScene(v_assimpScene);	// Start model loading from the Assimp scene structure

									//and finally generate tangents
									//generateTangents(v_Faces,temp_uvs,temp_normals);
	return true;
}

void ObjModel::unload()
{
	glDeleteBuffers((GLuint)BufferType::Num_Buffers, &m_VBOData[0]);
	glDeleteVertexArrays(1, &m_MeshLocation);
	m_Loaded = false;
}

unsigned int ObjModel::getHandle()
{
	return m_MeshLocation;
}

unsigned int ObjModel::getVertCount()
{
	return m_NumVerts;
}

void ObjModel::draw() const
{
	glBindVertexArray(m_MeshLocation);
	glDrawElements(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


ObjModel::~ObjModel()
{
	if (m_Loaded)
		unload();
}

void ObjModel::loadFromScene(const aiScene * p_assimpScene)
{
	// Reserve space in the vectors for every mesh
	m_MeshPool.resize(p_assimpScene->mNumMeshes);

	unsigned int v_numVerticesTotal = 0;
	unsigned int v_numIndicesTotal = 0;

	// Count the number of vertices and indices
	for (unsigned int i = 0; i < p_assimpScene->mNumMeshes; i++)					// For each mesh in the model
	{
		m_MeshPool[i].m_MaterialIndex = p_assimpScene->mMeshes[i]->mMaterialIndex;	// Get index for a material
		m_MeshPool[i].m_NumIndices = p_assimpScene->mMeshes[i]->mNumFaces * 3;		// Calculate the number of indices in the mesh

		m_MeshPool[i].m_BaseVertex = v_numVerticesTotal;	// Get base vertex and index, wich work similar as offset, when drawing from buffers
		m_MeshPool[i].m_BaseIndex = v_numIndicesTotal;

		v_numVerticesTotal += p_assimpScene->mMeshes[i]->mNumVertices;	// Calculate the total number of vertices
		v_numIndicesTotal += m_MeshPool[i].m_NumIndices;				// and indices
	}

	m_NumVerts = v_numIndicesTotal;

	// Reserve space in the vectors, to slighly improve performance of '.push_back's
	m_Vertices.reserve(v_numVerticesTotal);
	m_Normals.reserve(v_numVerticesTotal);
	m_Tangents.reserve(v_numVerticesTotal);
	m_Bitangents.reserve(v_numVerticesTotal);
	m_TextureCoordinates.reserve(v_numVerticesTotal);
	m_Indices.reserve(v_numIndicesTotal);

	// Deal with each mesh
	for (unsigned int i = 0; i < p_assimpScene->mNumMeshes; i++)
		loadFromMesh(p_assimpScene->mMeshes[i]);


}

void ObjModel::loadFromMesh(const aiMesh * p_assimpMesh)
{
	// Make sure that the texture coordinates array exist (by checking if the first member of the array does)
	bool v_textureCoordsExist = p_assimpMesh->mTextureCoords[0] ? true : false;

	// Put the vertex positions, normals, tangents, bitangents, texture coordinate data from Assimp data sets to our data sets (vectors)
	for (unsigned int i = 0; i < p_assimpMesh->mNumVertices; i++)
	{
		m_Vertices.push_back(glm::vec3(p_assimpMesh->mVertices[i].x, p_assimpMesh->mVertices[i].y, p_assimpMesh->mVertices[i].z));
		if (p_assimpMesh->mNormals != NULL)
			m_Normals.push_back(glm::vec3(p_assimpMesh->mNormals[i].x, p_assimpMesh->mNormals[i].y, p_assimpMesh->mNormals[i].z));
		if (p_assimpMesh->mTangents != NULL)
			m_Tangents.push_back(glm::vec3(p_assimpMesh->mTangents[i].x, p_assimpMesh->mTangents[i].y, p_assimpMesh->mTangents[i].z));
		if (p_assimpMesh->mBitangents != NULL)
			m_Bitangents.push_back(glm::vec3(p_assimpMesh->mBitangents[i].x, p_assimpMesh->mBitangents[i].y, p_assimpMesh->mBitangents[i].z));
		if (v_textureCoordsExist)	// Proceed with texture coordinates only if they exsist
			m_TextureCoordinates.push_back(glm::vec2(p_assimpMesh->mTextureCoords[0][i].x, p_assimpMesh->mTextureCoords[0][i].y));
	}

	// Populate the indices (from Assimp to our vectors)
	for (unsigned int i = 0; i < p_assimpMesh->mNumFaces; i++)
		if (p_assimpMesh->mFaces[i].mNumIndices == 3)
		{
			m_Indices.push_back(p_assimpMesh->mFaces[i].mIndices[0]);
			m_Indices.push_back(p_assimpMesh->mFaces[i].mIndices[1]);
			m_Indices.push_back(p_assimpMesh->mFaces[i].mIndices[2]);
		}
}

void ObjModel::initialise(void)
{
	//GLenum error = glGetError();
	if (!m_Loaded)
	{
		//create and enable new vertex array object
		glGenVertexArrays(1, &m_MeshLocation);
		glBindVertexArray(m_MeshLocation);
		//create a VBO for the vertex data
		if (m_Vertices.empty())
		{
			//no verts
			return;
		}

		//error = glGetError();
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, m_Indices.size() * sizeof(glm::vec3), m_Vertices.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)BufferType::Vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray((GLuint)BufferType::Vertex);
		m_VBOData[(GLuint)BufferType::Vertex] = VBO;

		//create a VBO for normal data
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, m_Indices.size() * sizeof(glm::vec3), m_Normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)BufferType::Normal, 3, GL_FLOAT, GL_FALSE, 0, 0);
		//error = glGetError();
		glEnableVertexAttribArray((GLuint)BufferType::Normal);
		//error = glGetError();
		m_VBOData[(GLuint)BufferType::Normal] = VBO;

		//create a VBO for uv data
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, m_Indices.size() * sizeof(float) * 2, m_TextureCoordinates.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)BufferType::TextureCoordinate, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray((GLuint)BufferType::TextureCoordinate);
		m_VBOData[(GLuint)BufferType::TextureCoordinate] = VBO;

		//create a VBO for tangent data
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, m_Indices.size() * sizeof(glm::vec3), m_Tangents.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)BufferType::Tangent, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray((GLuint)BufferType::Tangent);
		m_VBOData[(GLuint)BufferType::Tangent] = VBO;

		//create a VBO for bitangent data
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, m_Indices.size() * sizeof(glm::vec3), m_Bitangents.data(), GL_STATIC_DRAW);
		glVertexAttribPointer((GLuint)BufferType::BiTangent, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray((GLuint)BufferType::BiTangent);
		m_VBOData[(GLuint)BufferType::BiTangent] = VBO;

		//and finally a VBO for indices
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(GLuint), m_Indices.data(), GL_STATIC_DRAW);
		m_VBOData[(GLuint)BufferType::Index] = VBO;

		glBindVertexArray(0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		clearModelData();
		m_Loaded = true;
	}

}

void ObjModel::initialiseFromMeshData(const EC_TerrainMeshData& meshData)
{
	if (m_Loaded) return;
	if (meshData.positions.empty()) return;

	glGenVertexArrays(1, &m_MeshLocation);
	glBindVertexArray(m_MeshLocation);

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, meshData.positions.size() * sizeof(glm::vec3), meshData.positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)BufferType::Vertex, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray((GLuint)BufferType::Vertex);
	m_VBOData[(GLuint)BufferType::Vertex] = VBO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, meshData.normals.size() * sizeof(glm::vec3), meshData.normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)BufferType::Normal, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray((GLuint)BufferType::Normal);
	m_VBOData[(GLuint)BufferType::Normal] = VBO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(GLuint), meshData.indices.data(), GL_STATIC_DRAW);
	m_VBOData[(GLuint)BufferType::Index] = VBO;

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	m_NumVerts = static_cast<unsigned int>(meshData.indices.size());
	m_Loaded = true;
}

bool ObjModel::loadFromFile(std::string & filename, std::vector<std::string>& data)
{
	std::fstream fs;
	fs.open(filename);
	if (!fs.is_open())
		return false;
	while (!fs.eof())
	{
		char line[256];
		fs.getline(line, 256);
		data.push_back(std::string(line));
	}
	return true;
}

void ObjModel::clearModelData()
{
	m_Vertices.clear();
	m_TextureCoordinates.clear();
	m_Normals.clear();
	m_Tangents.clear();
	m_Bitangents.clear();
	m_Indices.clear();
}

