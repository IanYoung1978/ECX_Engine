#pragma once
#include "Subsystems/EC_System.h"
#include "Subsystems/EC_SystemType.h"
#include "TaskManager/EC_SystemTask.h"
#include "TaskManager/EC_ThreadManager.h"
#include <memory>
#include <vector>
#include "Messaging/ECXMessenger.h"
#include "Engine/Subsystems/Scripting/EC_VolumeAPI.h"

class GameEntity;
class EC_Game;
class EC_Event;
class EC_EventQueue;
class EC_VolumeNode;
class EC_AudioSystem;

class EC_Engine:
	public ICommandListener
{
public:
	EC_Engine();
	void init(const std::string& config, EC_Game& game, ECXMessenger& messenger);
	// Inherited via ICommandListener
	void receive(ECXCommand& command) override;
	void pause();
	void resume();
	// Runs every system's update() once, synchronously, on the calling thread -
	// independent of the threaded task loop and its pause flag. Used to bake a
	// correct initial transform/camera state (position, scale, view matrix)
	// before the very first render, since starting paused means the normal
	// per-tick loop may never run before that first frame is drawn.
	void stepOnce(float deltaTimeS);
	void start();
	void stop();
	// Forwards to the Scripting system's own runScriptOnce()/getVolumeRoot() - see
	// EC_LuaScriptSystem.h for what these do and why they're distinct from the normal
	// per-frame/per-event handler scripts.
	bool runLuaScriptOnce(const std::string& filename);
	std::shared_ptr<EC_VolumeNode> getVolumeRoot() const;
	ScriptAPI::VoxelTerrainConfig getVoxelTerrainConfig() const;
	// Forwards to the Audio system - see EC_AudioSystem.h for what each call does.
	void playSound(const std::string& path, float volume, const std::string& category);
	void playMusic(const std::string& path, float volume, bool loop);
	void stopMusic();
	void setCategoryVolume(const std::string& category, float volume);
	~EC_Engine();
private:
	EC_AudioSystem* getAudioSystem() const;
	std::vector<std::shared_ptr<EC_System>> m_Systems;
	std::vector<std::shared_ptr<EC_SystemTask>> m_tasks;
	EC_Game* m_game;
	EC_ThreadManager m_threadpool;

};

