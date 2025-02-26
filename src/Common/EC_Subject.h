#pragma once
#include <vector>
#include <memory>
#include <mutex>
class EC_Observer;
class EC_Subject
{
public:
	
	virtual ~EC_Subject();
	void Register(EC_Observer* obs);
	void UnRegister(EC_Observer* obs);
protected:
	EC_Subject();
	void notifyObservers();
	std::mutex m_lock;
private:
	std::vector<EC_Observer*> m_Observers;

};