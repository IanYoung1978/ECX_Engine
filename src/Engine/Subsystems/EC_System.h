#pragma once
#include <memory>
#include <mutex>
#include <vector>

class GameEntity;
class EC_Game;
class EC_Event;
class ECXMessenger;

class EC_System
{
public:
	EC_System();
	virtual void init(ECXMessenger& messenger, EC_Game& game) = 0;
	virtual void update(const float& deltaTimeS, EC_Game& game) = 0;
	virtual void addEntity(std::shared_ptr<GameEntity>& e) = 0;
	virtual void removeEntity(unsigned int entityID) = 0;
	virtual void clearEntities() = 0;
	virtual ~EC_System();
protected:
	std::mutex m_lock;
	bool m_Paused;
	std::vector<std::shared_ptr<GameEntity>> m_to_add;
	std::vector<unsigned int> m_to_remove;
};

