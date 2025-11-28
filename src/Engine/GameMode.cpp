#include "GameMode.h"
#include "Game.h"
#include "Graphics/GL_Deferred_Renderer.h"
#include "xml/XML.h"
#include "Logging/ECX_Logging.h"

EC_GameMode::EC_GameMode()
{
	m_game = nullptr;
}

void EC_GameMode::init(EC_Game & game,std::string& config, ECXMessenger& messenger)
{
	if (XML::loadGameModeSettings(config, m_settings))
	{
		m_engine.init(m_settings.engine_settings, game, messenger);
		m_game = &game;
	}
	messenger.Subscribe(*this, ECXCommandType::SystemStart);
	messenger.Subscribe(*this, ECXCommandType::SystemShutdown);
	m_scene_renderer = std::make_unique<GL_Deferred_Renderer>();
	m_scene_renderer->init(game.getWindow());
	m_loader = std::make_shared<EC_DOD_LoadingWorker>();
	m_ThreadManager.addTask(m_loader);
	m_ThreadManager.executeTasks();
	m_loader->scheduleScene(m_settings.game_world_data);

	m_loader->start();
}


void EC_GameMode::update(float deltaTimeS, EC_Game& game)
{
	// Check if loading worker needs finalization on main thread
	if (m_loader->needsFinalization())
	{
		m_loader->finalizeOnMainThread();
		LOGGING::ECX_Logger::GetInstance()->LogMessage(
			"Load complete!",
			LOGGING::LogLevel::INFORMATION
		);
	}

	// Optional: Display loading progress
	if (m_loader->isLoading())
	{
		float progress = m_loader->getProgress();
		// You can render a loading bar here: renderLoadingBar(progress);
	}

	m_scene_renderer->renderScene();
}

void EC_GameMode::openMenu()
{
	// add code for UI later
}

EC_GameMode::~EC_GameMode()
{
}

void EC_GameMode::receive(ECXCommand& command)
{
	if (command.type == ECXCommandType::SystemShutdown)
	{
		m_engine.pause();
		//EC_DOD_EntityManager::getInstance();
		m_engine.stop();
		m_loader->shutdown();
	}
	if (command.type == ECXCommandType::SystemStart)
	{
		m_engine.start();
	}
}

void EC_GameMode::changeMode(Game_Mode mode)
{
	m_game->changeMode(mode);
}

void EC_GameMode::startGame(EC_Game & game)
{
	game.startGame();
}

std::shared_ptr<Window> EC_GameMode::getWindow()
{
	return m_game->getWindow();
}
