#include "Game.h"
#include <memory>

int main(int argc, char *args[])
{
	auto game = std::make_unique<EC_Game>();
	Game_Error err = game->init("data/scripts/XML/Game.xml");
	if (err != Game_Error::NO_ERROR)
		return -1;
	err = game->run();
	if (err != Game_Error::NO_ERROR)
		return -2;
	return 0;
}