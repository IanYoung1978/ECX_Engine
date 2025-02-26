#include "EC_AddEntityCallback.h"
#include "GameMode.h"


EC_AddEntityCallback::EC_AddEntityCallback(EC_GameMode* mode): mode(nullptr)
{
	this->mode = mode;
}

EC_AddEntityCallback& EC_AddEntityCallback::operator=(EC_AddEntityCallback& other)
{
	mode = other.mode;
	return *this;
}

void EC_AddEntityCallback::execute(std::shared_ptr<GameEntity> entity)
{
	mode->addEntity(entity);
}
