#include "ProxyHelper.h"
size_t ProxyHelper::s_currentID = 0;
ProxyHelper::ProxyHelper()
{
}

size_t ProxyHelper::getUID()
{
	s_currentID++;
	return s_currentID;
}
