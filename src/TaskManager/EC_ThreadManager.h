#pragma once
#include <atomic>
#include <condition_variable>
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
	// For a caller that already holds getLock(): a worker has to test "is there work, or
	// am I stopping?" and wait() on that condition under ONE continuous hold of the lock -
	// releasing it between the check and the wait is exactly the window in which stop()'s
	// notification could be missed, leaving the worker asleep forever.
	bool hasTasksLocked() const { return !s_Tasks.empty(); }
	std::shared_ptr<EC_Task> popTaskLocked();
	std::condition_variable* getSignal();
	std::mutex* getLock();
	void stop();
	~EC_ThreadManager();
private:
	static std::vector<std::thread> s_Workers;
	static std::deque<std::shared_ptr<EC_Task>> s_Tasks;
	static std::mutex s_Lock;
	static std::condition_variable s_Flag;
	static std::atomic<bool> s_running;
	static size_t s_activeThreads;
};
