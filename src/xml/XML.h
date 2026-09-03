#pragma once

#include "xml/tinyxml.h"
#include "Window/WindowSettings.h"
#include "Engine/Config.h"
#include "Engine/GameModeSettings.h"
#include "Graphics/Renderers/RenderConfig.h"
#include <string>
#include <map>
#include <algorithm>
#include <cstdlib>
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
				else if (strcmp(child->Value(), "UI") == 0)
				{
					settings.ui_file = child->FirstAttribute()->Value();
				}
				else if (strcmp(child->Value(), "PhysicsMaterials") == 0)
				{
					settings.physics_materials_file = child->FirstAttribute()->Value();
				}
				child = child->NextSiblingElement();
			}
			game = game->NextSiblingElement();
		}
		return true;
	}

	// EngineConfig.xml's <Physics><Debug> flags - one independent toggle per
	// physics attribute (see EC_PhysicsSystem::setDebugLogging), rather than
	// a single all-or-nothing switch, so e.g. friction can be traced without
	// drowning it in a velocity line for every body every tick.
	struct PhysicsDebugSettings
	{
		bool logEnergy = false;
		bool logVelocity = false;
		bool logAngularVelocity = false;
		bool logFriction = false;
	};

	// Parses EngineConfig.xml's <Physics><Debug> flags. Missing file/
	// section/tag all quietly default to false rather than failing, since
	// these are optional debug toggles, not required settings.
	inline bool loadPhysicsDebugSettings(const std::string& file, PhysicsDebugSettings& outSettings)
	{
		outSettings = PhysicsDebugSettings{};

		TiXmlDocument doc(file.c_str());
		if (!doc.LoadFile())
			return false;
		auto root = doc.FirstChildElement();
		if (!root)
			return false;

		auto physics = root->FirstChildElement("Physics");
		if (!physics)
			return true;
		auto debug = physics->FirstChildElement("Debug");
		if (!debug)
			return true;

		auto readFlag = [&](const char* tagName, bool& out)
		{
			auto elem = debug->FirstChildElement(tagName);
			if (elem && elem->GetText())
				out = (strcmp(elem->GetText(), "true") == 0);
		};
		readFlag("LogEnergy", outSettings.logEnergy);
		readFlag("LogVelocity", outSettings.logVelocity);
		readFlag("LogAngularVelocity", outSettings.logAngularVelocity);
		readFlag("LogFriction", outSettings.logFriction);

		return true;
	}

	// Parses EngineConfig.xml's <Physics><Substeps> - how many times
	// Collision+Physics re-run per visual tick (see
	// EC_PhysicsThreadTask::setSubstepCount). Missing file/section/tag
	// default to 1 (no substepping, original behaviour) rather than
	// failing.
	inline bool loadPhysicsSubstepCount(const std::string& file, int& outSubsteps)
	{
		outSubsteps = 1;

		TiXmlDocument doc(file.c_str());
		if (!doc.LoadFile())
			return false;
		auto root = doc.FirstChildElement();
		if (!root)
			return false;

		auto physics = root->FirstChildElement("Physics");
		if (!physics)
			return true;
		auto substeps = physics->FirstChildElement("Substeps");
		if (substeps && substeps->GetText())
			outSubsteps = std::max(1, atoi(substeps->GetText()));

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
			else if (strcmp(child->Value(), "EmissiveIntensity") == 0 && child->GetText())
			{
				settings.emissiveIntensity = std::stof(child->GetText());
			}
			else if (strcmp(child->Value(), "BloomMipLevels") == 0 && child->GetText())
			{
				settings.bloomMipLevels = std::stoi(child->GetText());
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
			else if (strcmp(child->Value(), "DirShadow") == 0)
			{
				const char* distance = child->Attribute("distance");
				if (distance) settings.dirShadowDistance = std::stof(distance);
			}
			child = child->NextSiblingElement();
		}
		return true;
	}
}