#include "EC_Subject.h"
#include "EC_Observer.h"

EC_Subject::~EC_Subject()
{
	m_Observers.clear();
}

void EC_Subject::Register(EC_Observer * obs)
{

	//std::scoped_lock<std::mutex> lock(m_lock);
	m_Observers.push_back(obs);
}

void EC_Subject::UnRegister(EC_Observer * obs)
{
	//std::scoped_lock<std::mutex> lock(m_lock);
	for (size_t i = 0; i < m_Observers.size(); i++)
	{
		if (obs == m_Observers[i])
		{
			m_Observers[i] = m_Observers.back();
			m_Observers.resize(m_Observers.size() - 1);
			break;
		}
	}
}

EC_Subject::EC_Subject()
{
	m_Observers.clear();
}

void EC_Subject::notifyObservers()
{
	//std::scoped_lock<std::mutex> lock(m_lock);
	if (!m_Observers.empty())
	{

		for (auto obs : m_Observers)
		{
			obs->notify();
		}
	}
}
