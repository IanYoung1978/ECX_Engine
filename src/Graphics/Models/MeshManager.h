#pragma once
#include <string>
#include <memory>
#include <map>
#include <mutex>

class ObjModel;
class MeshManager
{
public:
	MeshManager();
	void loadObjModel(const std::string& fname);
	std::shared_ptr<ObjModel> getObjModel(const std::string& fname);
	void finaliseModels();
	std::shared_ptr<ObjModel> finaliseModel(const std::string& fname);
	~MeshManager();
private:
	std::shared_ptr<ObjModel> findObjModel(const std::string& fname);
	std::map<std::string, std::shared_ptr<ObjModel>> m_models;
	std::mutex m_Mutex;
};

