#include "EC_Observer.h"
#include "EC_Subject.h"
EC_Observer::~EC_Observer()
{
}

EC_Observer::EC_Observer(EC_Subject * subject)
{
	if (subject != nullptr)
	{
		subject->Register(this);
	}
}
