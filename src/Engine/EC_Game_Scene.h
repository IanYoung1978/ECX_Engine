#pragma once
#include <string>
class EC_Game;
class EC_Game_Scene
{
public:
	EC_Game_Scene();
	bool load(std::string& filename, EC_Game& game);
	void unload();
	~EC_Game_Scene();
};

