#pragma once
#include "EC_Task.h"
#include <string>
#include <deque>
#include <mutex>
#include "Entity/EntityFactory.h"
#include "Engine/EC_AddEntityCallback.h"

class EC_GameMode;

class EC_File_IO_Task :
	public EC_Task
{
public:
	EC_File_IO_Task();
	virtual ~EC_File_IO_Task();
	// Public API

	// Schedules an entity descriptor to be loaded from hdd
	void ScheduleloadEntity(const std::string& filename, EC_AddEntityCallback cb);
	// schedules an entire scene(game world) to be loaded from hdd
	void ScheduleloadScene(const std::string& filename, EC_AddEntityCallback cb);
	// When all assets are loaded, any gpu assets (VBO etc) will be created on main thread.
	void finalize();
	// start the task
	void start(EC_GameMode* mode);
	// abort loading
	void abort();
	//shut down the task if shutting down game
	void shutdown();
	// get the progress of loading.
	float getProgress();
	bool isLoading();
	bool hasLoaded();
	std::vector<std::shared_ptr<GameEntity>>& getCameras();
	std::vector<std::shared_ptr<GameEntity>>& getEntities();
	std::vector<std::shared_ptr<GameEntity>>& getLights();
	// Inherited via EC_Task
	virtual void execute() override;
private:
	int loadEntity(const std::string& filename);
	int loadSceneFile(const std::string& filename);
	std::deque<std::string> m_scheduled_files_load;
	std::deque<std::string> m_scheduled_scene_load;
	bool m_running;
	bool m_load_in_progress;
	bool m_full_Scene;
	bool m_sceneLoaded;
	unsigned int m_progress;
	size_t m_item_scheduled_count;
	unsigned int m_item_loaded_count;
	float m_loaded_percent;
	EntityFactory m_factory;
	EC_GameMode* m_current_mode;
	std::mutex m_lock;
	EC_AddEntityCallback callback;
};

