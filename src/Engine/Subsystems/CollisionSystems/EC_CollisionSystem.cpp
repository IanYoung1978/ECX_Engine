#include "EC_CollisionSystem.h"
#include "Messaging/ECXMessenger.h"
EC_CollisionSystem::EC_CollisionSystem()
{
}
EC_CollisionSystem::~EC_CollisionSystem()
{
}
void EC_CollisionSystem::init(ECXMessenger& messenger, EC_Game& game)
{
	m_Messenger = &messenger;
	m_BroadPhase = std::make_unique<EC_BroadPhase>();
	m_BroadPhase->init(messenger);
	m_NarrowPhase = std::make_unique<EC_NarrowPhase>();
	m_NarrowPhase->init(messenger);

}
void EC_CollisionSystem::update(const float& deltaTimeS, EC_Game& game)
{
	m_BroadPhase->broadPhaseCollisionDetection();
	m_NarrowPhase->narrowPhaseCollisionDetection();
	m_PairManager.update();
}
