#include "Game.h"
#include "Window/SDL_GL_Window.h"
#include "xml/XML.h"
#include "Engine/Keyboard.h"
#include "Engine/Config.h"
#include "Engine/GameMode.h"
#include "Logging/ECX_Logging.h"


EC_Game::EC_Game() :m_Running(true)
{
	m_CurrentMode = 0;
	m_Accumulator = 0.0f;
}

Game_Error EC_Game::init(const std::string& configurationFilename)
{
	//Load Game config file
	GameSettings game_settings;
	LOGGING::ECX_Logger::GetInstance()->setOutputFilename("log.html");
	if (!XML::loadGameConfig(configurationFilename, game_settings))
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Config error", LOGGING::LogLevel::CRITICAL);
		return Game_Error::CONFIG_ERROR;
	}
	//load window/opengl settings from xml file
	WindowSettings settings;
	if (!XML::createWindowSettings(game_settings.window_settings_file, settings))
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Config error", LOGGING::LogLevel::CRITICAL);
		return Game_Error::CONFIG_ERROR;
	}
	//create window based on loaded settings
	m_Window = std::make_shared<SDL_GL_Window>();
	if (!m_Window->init(settings))
	{
		LOGGING::ECX_Logger::GetInstance()->LogMessage("Window error", LOGGING::LogLevel::CRITICAL);
		return Game_Error::WINDOW_ERROR;
	}
		
	//create a controller
	m_Controls = std::make_shared<ControlSystem>();
	m_Controls->init(m_Messenger,*this);
	for (auto s : game_settings.GameModes)
	{
		//create game modes
		auto mode = std::make_unique<EC_GameMode>();
		m_Modes.push_back(std::move(mode));
		m_Modes.back()->init(*this, s, m_Messenger);
	}

	//init m_Controls
	m_Timer = std::make_unique<Timer>();
	m_threadmanager.init(8);
	m_Running = true;
	m_Messenger.Subscribe(*this, ECXCommandType::SystemShutdown);
	LOGGING::ECX_Logger::GetInstance()->LogMessage("Init complete", LOGGING::LogLevel::INFORMATION);
	return Game_Error::NO_ERROR;
}

Game_Error EC_Game::run()
{
	int timer = 0;
	SDL_Event e;
	m_CurrentMode = 0;
	ECXCommand command;
	command.type = ECXCommandType::SystemStart;
	m_Messenger.publish(command);
	while (m_Running)
	{
		//poll input events
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				m_Running = false;
			//use controllers to store input data
			m_Controls->handleEvent(e);
		}
		//update
		m_Timer->update(*this);
	}
	m_Window->close();
	LOGGING::ECX_Logger::GetInstance()->LogMessage("Shutting down", LOGGING::LogLevel::INFORMATION);
	LOGGING::ECX_Logger::GetInstance()->printToFile();
	return Game_Error::NO_ERROR;
}

void EC_Game::addEntity(std::shared_ptr<GameEntity> e)
{
	std::scoped_lock<std::mutex> lock(m_lock);
}

void EC_Game::removeEntity(unsigned int gameEntityID)
{
	std::scoped_lock<std::mutex> lock(m_lock);
}

std::shared_ptr<GameEntity> EC_Game::getEntityByName(const std::string & eName)
{
	std::scoped_lock<std::mutex> lock(m_lock);
	//find the first entity with specified name
	
	return m_Modes[m_CurrentMode]->getEntity(eName);
}

void EC_Game::clearEntities()
{
	m_Modes[m_CurrentMode]->clearEntities();
}

void EC_Game::shutDown()
{
	ECXCommand command;
	command.type = ECXCommandType::SystemShutdown;
	m_Messenger.publish(command);
}

void EC_Game::update(const float & deltaTimeS)
{
	if (m_Running)
	{
		m_Messenger.flush();
		m_Modes[m_CurrentMode]->update(deltaTimeS, *this);
		m_Controls->update(deltaTimeS, *this);
		m_Window->present();
	}
}

EC_Game::~EC_Game()
{
}

void EC_Game::changeMode(Game_Mode mode)
{
	m_CurrentMode = (size_t)mode;
}

KeyState EC_Game::getKeyState(SDL_Scancode key)
{
	return m_Controls->getKeyState(key);
}

void EC_Game::startGame()
{
}

std::shared_ptr<Window> EC_Game::getWindow()
{
	return m_Window;
}

std::shared_ptr<ControlSystem> EC_Game::getControls()
{
	return m_Controls;
}

void EC_Game::receive(ECXCommand& command)
{
	if (command.type == ECXCommandType::SystemShutdown)
	{
		m_Running = false;
		m_Controls->shutdown();
		m_threadmanager.stop();
	}
}
