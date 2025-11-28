#ifndef ECX_COMMAND_BROKER_H
#define ECX_COMMAND_BROKER_H
#include <map>
#include <vector>
#include "ECXCommand.h"
#include <deque>
#include <mutex>

class ECXCommand;
class ICommandListener;
enum class ECXCommandType;



class ECXCommandBroker
{
public:
	ECXCommandBroker();
	void Subscribe(ICommandListener& listener, ECXCommandType type);
	void publish(ECXCommand& Command);
	void flush();
private:
	int dequeSelector;
	std::deque<ECXCommand> queues[2];
	std::mutex pushLock;
	std::map<ECXCommandType, std::vector<ICommandListener*>> listeners;
};
#endif // !ECX_COMMAND_BROKER_H
