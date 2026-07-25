#pragma once

#include "xml/tinyxml.h"
#include "Window/WindowSettings.h"
#include "Engine/Config.h"
#include "Engine/GameModeSettings.h"
#include "Graphics/RenderConfig.h"
#include <string>
#include <map>
#include <SDL.h>
#include "Logging/ECX_Logging.h"

namespace XML
{
	struct SceneDescriptor
	{
		std::string alias;
		std::string filename;
		bool precache = false;
		bool unloadOnDeactivate = true;
	};

	inline bool loadScenesFile(const std::string& file, std::vector<SceneDescriptor>& scenes)
	{
		TiXmlDocument doc(file.c_str());
		if (!doc.LoadFile())
			return false;
		auto root = doc.FirstChildElement();
		if (!root || strcmp(root->Value(), "Scenes") != 0)
			return false;

		auto child = root->FirstChildElement("Scene");
		while (child)
		{
			SceneDescriptor desc;
			const char* alias = child->Attribute("alias");
			if (alias) desc.alias = alias;
			const char* precache = child->Attribute("precache");
			if (precache) desc.precache = (strcmp(precache, "true") == 0);
			const char* unload = child->Attribute("unloadondeactivate");
			if (unload) desc.unloadOnDeactivate = (strcmp(unload, "true") == 0);

			if (child->GetText()) desc.filename = child->GetText();
			scenes.push_back(desc);
			child = child->NextSiblingElement("Scene");
		}
		return true;
	}

	inline bool loadKeyMapping(const std::string& file, std::map<SDL_Scancode, std::string>& mappings)
	{
		TiXmlDocument doc(file.c_str());
		if (!doc.LoadFile())
			return false;
		auto elem = doc.FirstChildElement();
		if (!elem)
			return false;

		while (elem)
		{
			if (elem->ValueTStr() == "KeyMapping")
			{
				auto child = elem->FirstChildElement();
				while (child)
				{
					auto att = child->FirstAttribute();
					if (att && strcmp(att->Name(), "action") == 0)
					{
						std::string key(child->GetText());
						SDL_Scancode code = SDL_GetScancodeFromName(key.c_str());
						if (code != SDL_SCANCODE_UNKNOWN)
						{
							mappings.emplace(code, att->Value());
						}
						else
						{
							LOGGING::ECX_Logger::GetInstance()->LogMessage("unknown control mapping: ", LOGGING::LogLevel::SEVERE);
						}
					}
					child = child->NextSiblingElement();
				}
			}
			elem = elem->NextSiblingElement();
		}
		return true;
	}
	inline bool loadGameModeSettings(const std::string& file, GameModeSettings& settings)
	{
		TiXmlDocument doc(file.c_str());
		if (!doc.LoadFile())
			return false;
		auto game = doc.FirstChildElement();
		if (!game)
			return false;
		while (game)
		{
			auto child = game->FirstChildElement();
			while (child)
			{
				if (strcmp(child->Value(), "EngineSettings") == 0)
				{
					settings.engine_settings = child->FirstAttribute()->Value();
				}
				else if (strcmp(child->Value(), "Controls") == 0)
				{
					//game controls data
					settings.controls = child->FirstAttribute()->Value();
				}
				else if (strcmp(child->Value(), "Scenes") == 0)
				{
					//game world data
					settings.scenes_file = child->FirstAttribute()->Value();
				}
				else if (strcmp(child->Value(), "GraphicsSettings") == 0)
				{
					settings.graphics_settings = child->FirstAttribute()->Value();
				}
				child = child->NextSiblingElement();
			}
			game = game->NextSiblingElement();
		}
		return true;
	}

	inline bool loadGameConfig(const std::string& file, GameSettings& settings)
	{
		TiXmlDocument doc(file.c_str());
		if (!doc.LoadFile())
			return false;
		auto game = doc.FirstChildElement();
		if (!game)
			return false;
		while (game)
		{
			auto child = game->FirstChildElement();
			while (child)
			{
				if (strcmp(child->Value(), "Window") == 0)
				{
					settings.window_settings_file = child->FirstAttribute()->Value();
				}
				else
				{
					//game mode data
					settings.GameModes.push_back(child->FirstAttribute()->Value());
				}
				child = child->NextSiblingElement();
			}
			game = game->NextSiblingElement();
		}
		return true;

	}
	inline bool createWindowSettings(const std::string& settingsFile, WindowSettings& settings)
	{
		TiXmlDocument doc(settingsFile.c_str());
		if (!doc.LoadFile())
			return false;

		auto elem = doc.FirstChildElement();
		if (!elem)
			return false;
		if (strcmp(elem->Value(), "Window") == 0)
		{
			auto child = elem->FirstChildElement();
			while (child != NULL)
			{
				
				if (strcmp(child->Value(), "width") == 0)
				{
					settings.width = std::stoi(child->GetText());
				}
				else if (strcmp(child->Value(), "height") == 0)
				{
					settings.height = std::stoi(child->GetText());
				}
				else if (strcmp(child->Value(), "fullscreen") == 0)
				{
					settings.fullscreen = (strcmp(child->GetText(), "true")==0) ? true : false;
				}
				else if (strcmp(child->Value(), "glMajor") == 0)
				{
					settings.OGLMajor = std::stoi(child->GetText());
				}
				else if (strcmp(child->Value(), "glMinor") == 0)
				{
					settings.OGLMinor = std::stoi(child->GetText());
				}
				else if (strcmp(child->Value(), "AA") == 0)
				{
					settings.MSAALevel = std::stoi(child->GetText());
					settings.multisampling = true;
				}
				else if (strcmp(child->Value(), "WindowName") == 0)
				{
					settings.windowName = std::string(child->GetText());
				}
				else
				{
					return false; //malformed tag
				}
				child = child->NextSiblingElement();
			}
		}
		return true;
	}
	inline bool loadRenderConfig(const std::string& file, RenderConfig& settings)
	{
		TiXmlDocument doc(file.c_str());
		if (!doc.LoadFile())
			return false;
		auto root = doc.FirstChildElement();
		if (!root)
			return false;
		auto child = root->FirstChildElement();
		while (child)
		{
			if (strcmp(child->Value(), "Exposure") == 0 && child->GetText())
			{
				settings.exposure = std::stof(child->GetText());
			}
			else if (strcmp(child->Value(), "ShadowAtlas") == 0)
			{
				const char* size = child->Attribute("size");
				if (size) settings.shadowAtlasSize = std::stoi(size);
				const char* tileSize = child->Attribute("tileSize");
				if (tileSize) settings.shadowAtlasTileSize = std::stoi(tileSize);
			}
			else if (strcmp(child->Value(), "PointShadowPool") == 0)
			{
				const char* size = child->Attribute("size");
				if (size) settings.pointShadowPoolSize = std::stoi(size);
				const char* faceSize = child->Attribute("faceSize");
				if (faceSize) settings.pointShadowFaceSize = std::stoi(faceSize);
			}
			child = child->NextSiblingElement();
		}
		return true;
	}
}