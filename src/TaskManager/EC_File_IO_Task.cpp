#include "EC_File_IO_Task.h"
#include <tinyxml.h>
#include "Engine/GameMode.h"
#include "Logging/ECX_Logging.h"

EC_File_IO_Task::EC_File_IO_Task(): 
	m_current_mode(nullptr), 
	m_full_Scene(false), 
	m_item_scheduled_count(0), 
	m_loaded_percent(0),
	m_progress(0),
	m_sceneLoaded(false),
	m_item_loaded_count(0),
	m_load_in_progress(false)
{
	m_running = true;
}

EC_File_IO_Task::~EC_File_IO_Task()
{
	m_scheduled_files_load.clear();
}

void EC_File_IO_Task::ScheduleloadEntity(const std::string & filename, EC_AddEntityCallback cb)
{
	std::unique_lock<std::mutex> lock(m_lock);
	callback = cb;
	m_scheduled_files_load.push_back(filename);

}

void EC_File_IO_Task::ScheduleloadScene(const std::string & filename, EC_AddEntityCallback cb)
{
	std::unique_lock<std::mutex> lock(m_lock);
	callback = cb;
	m_full_Scene = true;
	m_scheduled_scene_load.push_back(filename);
}

void EC_File_IO_Task::finalize()
{
	m_load_in_progress = false;
	m_full_Scene = false;
	m_running = false;
}

void EC_File_IO_Task::start(EC_GameMode* mode)
{
	std::unique_lock<std::mutex> lock(m_lock);
	m_load_in_progress = true;
	m_loaded_percent = 0;
	m_current_mode = mode;
}

void EC_File_IO_Task::abort()
{
	std::unique_lock<std::mutex> lock(m_lock);
	m_running = false;
	m_scheduled_files_load.clear();
	m_full_Scene = false;
}

float EC_File_IO_Task::getProgress()
{
	return m_loaded_percent;
}

void EC_File_IO_Task::shutdown()
{
	std::unique_lock<std::mutex> lock(m_lock);
	m_running = false;
}

bool EC_File_IO_Task::isLoading()
{
	return m_load_in_progress;
}

bool EC_File_IO_Task::hasLoaded()
{
	return m_sceneLoaded;
}

std::vector<std::shared_ptr<GameEntity>>& EC_File_IO_Task::getCameras()
{
	return m_factory.getCameras();
}

std::vector<std::shared_ptr<GameEntity>>& EC_File_IO_Task::getEntities()
{
	return m_factory.getEntities();
}

std::vector<std::shared_ptr<GameEntity>>& EC_File_IO_Task::getLights()
{
	return m_factory.getLights();
}

void EC_File_IO_Task::execute()
{
	while (m_running)
	{

		// if we have a scheduled scene to load, load it
		// else if we have a scheduled entity to load, load it.
		if (!m_scheduled_scene_load.empty())
		{
			// if we are loading a full scene
			// load the scene descriptor file and 
			// begin loading entities
			m_sceneLoaded = false;
			std::string m_scheduled_scene = m_scheduled_scene_load.front();
			m_scheduled_scene_load.pop_front();
			if (loadSceneFile(m_scheduled_scene) == -1)
			{
				LOGGING::ECX_Logger::GetInstance()->LogMessage("Failed to load scene: " + m_scheduled_scene, LOGGING::LogLevel::CRITICAL);
				return;
			}
		}
		if (!m_scheduled_files_load.empty())
		{
			m_item_scheduled_count = m_scheduled_files_load.size();
			if (m_item_scheduled_count == 0)
			{
				// if there are no entities to load, go back to sleep
				m_load_in_progress = false;
				break;
			}
			m_item_loaded_count = 0;
			while (!m_scheduled_files_load.empty())
			{
				auto ent = m_scheduled_files_load.front();
				{
					std::scoped_lock<std::mutex> lock(m_lock);
					m_scheduled_files_load.pop_front();
				}
				if (loadEntity(ent) != -1)
				{

					m_item_loaded_count++;
					m_loaded_percent = (100 * ((float)m_item_loaded_count / m_item_scheduled_count));
					if (m_loaded_percent >= 100)
					{
						std::unique_lock<std::mutex> lock(m_lock);
						m_load_in_progress = false;
					}
				}
			}
		}
	}
}

int EC_File_IO_Task::loadEntity(const std::string & filename)
{
	// load entity xml file
	// use texture manager to load gfx material
	// use mesh manager to load model
	// use shader manager to load shaders
	// construct and store entity
	// any errors return -1;
	TiXmlDocument doc(filename.c_str());
	if (!doc.LoadFile())
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Failed to load entity: " + filename, LOGGING::LogLevel::TRIVIAL);
		return -1;
	}
	callback.execute(m_factory.constructEntity(*doc.FirstChildElement()));
	LOGGING::ECX_Logger::GetInstance()->LogMessage("Loaded: " + filename, LOGGING::LogLevel::INFORMATION);
	return 0;
}

int EC_File_IO_Task::loadSceneFile(const std::string & filename)
{
	// load xml file for scene
	// populate list of entities to load
	TiXmlDocument doc(filename.c_str());
	if (!doc.LoadFile())
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Failed to load scene file: " + filename, LOGGING::LogLevel::CRITICAL);
		m_running = false;
		return -1;
	}
	LOGGING::ECX_Logger::GetInstance()->LogMessage("Loading: " + filename, LOGGING::LogLevel::INFORMATION);
	// get entity descriptors
	auto root = doc.FirstChildElement();
	if (!root)
		return -1;
	if (strcmp(root->Value(), "Scene") != 0)
		return -1;
	auto elem = root->FirstChildElement();
	while (elem)
	{
		if (strcmp(elem->Value(), "Entity") == 0)
		{
			auto file = elem->FirstChildElement();
			if (file != NULL)
			{
				if (strcmp(file->Value(), "Filename") == 0)
				{
					std::scoped_lock<std::mutex> lock(m_lock);
					m_scheduled_files_load.emplace_back(file->GetText());
				}
				else
				{
					callback.execute(m_factory.constructEntity(*elem));
				}
			}
			else
				return -1;
		}
		elem = elem->NextSiblingElement();
	}
	LOGGING::ECX_Logger::GetInstance()->LogMessage("Loaded: " + filename, LOGGING::LogLevel::INFORMATION);

	return 0;
}
