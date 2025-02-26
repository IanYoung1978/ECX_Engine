#pragma once
class EC_Subject;
class EC_Observer 
{
public:
	virtual ~EC_Observer();
	virtual void notify() = 0;
	virtual bool operator == (EC_Observer& other) = 0;
protected:
	EC_Observer(EC_Subject* subject = nullptr);
};