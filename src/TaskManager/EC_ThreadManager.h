#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include "EC_Task.h"

class EC_ThreadManager
{
public:
	EC_ThreadManager();
	int init(size_t num_threads);
	std::shared_ptr<EC_Task> getTask();
	void addTask(std::shared_ptr<EC_Task> task);
	void executeTasks();
	bool hasTasks();
	bool running();
	std::condition_variable* getSignal();
	std::mutex* getLock();
	void stop();
	~EC_ThreadManager();
private:
	static std::vector<std::thread> s_Workers;
	static std::deque<std::shared_ptr<EC_Task>> s_Tasks;
	static std::mutex s_Lock;
	static std::condition_variable s_Flag;
	static bool s_running;
	static int s_activeThreads;
};
