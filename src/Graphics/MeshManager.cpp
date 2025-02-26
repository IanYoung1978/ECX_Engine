#include "MeshManager.h"
#include "ObjModel.h"


MeshManager::MeshManager()
{
}

void MeshManager::loadObjModel(const std::string & fname)
{
	auto model = findObjModel(fname);
	if (model == nullptr)
	{
		auto m = std::make_shared<ObjModel>();
		if (m->loadModel(fname))
		{
			m_models.emplace(fname, m);
		}
	}
}

std::shared_ptr<ObjModel> MeshManager::getObjModel(const std::string & fname)
{
	return findObjModel(fname);
}

void MeshManager::finaliseModels()
{
	for (auto model : m_models)
	{
		model.second->initialise();
	}
}

std::shared_ptr<ObjModel> MeshManager::finaliseModel(const std::string& fname)
{
	// This should always find a model
	auto m = findObjModel(fname);
	if (m == nullptr)
	{
		// log error
		return nullptr;
	}
	m->initialise();
	return m;
}


MeshManager::~MeshManager()
{
}

std::shared_ptr<ObjModel> MeshManager::findObjModel(const std::string & fname)
{
	auto model = m_models.find(fname);
	if (model != m_models.end())
	{
		return model->second;
	}
	return nullptr;
}
