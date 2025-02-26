#pragma once
class ProxyHelper
{
public:
	ProxyHelper();
	static size_t getUID();
private:
	static size_t s_currentID;
};

