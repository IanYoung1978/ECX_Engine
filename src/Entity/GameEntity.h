#pragma once
#include "Components/IComponent.h"
#include <vector>
#include <map>
#include <string>
#include <typeindex>
static const unsigned int MAX_ENTITIES = 2048;
class GameEntity
{
public:
	GameEntity();
	template <typename T>
	std::shared_ptr<T> getComponent() 
	{
		auto it = m_Components.find(std::type_index(typeid(T)));
		if (it != m_Components.end())
			return std::dynamic_pointer_cast<T>(it->second);
		return nullptr;
	}
	void addComponent(std::type_index type, std::shared_ptr<IComponent> component);
	template <typename T>
	bool hasComponent()
	{
		auto it = m_Components.find(std::type_index(typeid(T)));
		if (it != m_Components.end())
			return true;
		return false;
	}
	void setName(std::string name);
	std::string getName();
	unsigned int getUID();
	void setUID(unsigned int uid);
	bool isActive() { return m_Active; }
	void activate() { m_Active = true; }
	void deactivate() { m_Active = false; }
	void addChild(unsigned int childUID) { m_Children.push_back(childUID); }
	std::vector<unsigned int>& getChildren() { return m_Children; }
	virtual ~GameEntity();
private:
	std::map<std::type_index, std::shared_ptr<IComponent>> m_Components;
	std::vector<unsigned int> m_Children;
	std::string m_Name;
	bool m_Active;
	unsigned int m_UID;
};

