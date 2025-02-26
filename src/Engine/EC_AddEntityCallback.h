#pragma once
#include <memory>

class EC_GameMode;
class GameEntity;
class EC_AddEntityCallback
{
public:
	EC_AddEntityCallback() :mode(nullptr) { ; }
	EC_AddEntityCallback(EC_GameMode* mode);
	EC_AddEntityCallback &operator =(EC_AddEntityCallback& other);
	void execute(std::shared_ptr<GameEntity> entity);
private:
	EC_GameMode* mode;
};

