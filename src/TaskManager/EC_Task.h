#pragma once

class EC_Task
{
public:
	EC_Task() { ; }
	virtual ~EC_Task() { ; }
	virtual void execute() = 0;
};