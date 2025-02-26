#include "GameMode.h"
#include "Game.h"
#include "Graphics/GL_Deferred_Renderer.h"
#include "xml/XML.h"
#include "EC_AddEntityCallback.h"
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
	m_loader = std::make_shared<EC_File_IO_Task>();
	m_ThreadManager.addTask(m_loader);
	m_ThreadManager.executeTasks();
	m_loader->ScheduleloadScene(m_settings.game_world_data, EC_AddEntityCallback(this));

	m_loader->start(this);
}


void EC_GameMode::update(float deltaTimeS, EC_Game & game)
{
	if (m_loader->isLoading())
	{
		if (m_loader->hasLoaded())
		{
			LOGGING::ECX_Logger::GetInstance()->LogMessage("load complete ", LOGGING::LogLevel::INFORMATION);
		}
	}

	m_scene_renderer->renderScene();
}

void EC_GameMode::addEntity(std::shared_ptr<GameEntity>& entity)
{
	m_entities.push_back(entity);
	m_scene_renderer->addEntity(entity);
	m_engine.addEntity(entity);
}

std::shared_ptr<GameEntity> EC_GameMode::getEntity(const std::string& name)
{
	for (auto e : m_entities)
	{
		if (e->getName() == name)
		{
			return e;
		}
	}
	return nullptr;
}

std::shared_ptr<GameEntity> EC_GameMode::getEntity(unsigned int id)
{
	for (auto e : m_entities)
	{
		if (e->getUID() == id)
		{
			return e;
		}
	}
	return nullptr;
}

void EC_GameMode::removeEntity(const std::string & name)
{
	for (size_t i = 0; i < m_entities.size(); i++)
	{
		if (m_entities[i]->getName() == name)
		{
			auto e = m_entities[i];
			m_entities[i] = m_entities.back();
			m_entities.resize(m_entities.size() - 1);
			m_engine.removeEntity(e->getUID());
		}
	}
}

void EC_GameMode::removeEntity(unsigned int id)
{
	for (size_t i = 0; i < m_entities.size(); i++)
	{
		if (m_entities[i]->getUID() == id)
		{
			m_entities[i] = m_entities.back();
			m_entities.resize(m_entities.size() - 1);
			m_engine.removeEntity(id);
		}
	}
}

void EC_GameMode::clearEntities()
{
	m_entities.clear();
	m_engine.clearEntities();
}

void EC_GameMode::reset()
{
	m_engine.clearEntities();
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
		m_engine.clearEntities();
		m_scene_renderer->clearScene();
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
