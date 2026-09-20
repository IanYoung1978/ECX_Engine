#include "EC_ThreadManager.h"
#include <SDL_cpuinfo.h>
#include <string>

int EC_ThreadFunction(EC_ThreadManager* manager)
{
	while (true)
	{
		std::shared_ptr<EC_Task> task;
		{
			// Check-and-wait under one continuous hold of the lock, with the predicate
			// re-tested by wait() itself: a stop() (or a new task) that lands at any point
			// before the wait is seen by the predicate, and one that lands after is a real
			// notification to a thread that is genuinely waiting - never a lost wakeup.
			// Also copes with spurious wakeups, which a bare wait() did not.
			std::unique_lock<std::mutex> lock(*(manager->getLock()));
			manager->getSignal()->wait(lock, [manager] {
				return !manager->running() || manager->hasTasksLocked();
			});
			if (!manager->running())
			{
				break;
			}
			task = manager->popTaskLocked();
		}
		if (task != nullptr)
		{
			task->execute();
		}
	}
	return 0;
}

std::vector<std::thread> EC_ThreadManager::s_Workers;
std::deque<std::shared_ptr<EC_Task>> EC_ThreadManager::s_Tasks;
std::mutex EC_ThreadManager::s_Lock;
std::condition_variable EC_ThreadManager::s_Flag;
std::atomic<bool> EC_ThreadManager::s_running;
size_t EC_ThreadManager::s_activeThreads;

EC_ThreadManager::EC_ThreadManager()
{

}

int EC_ThreadManager::init(size_t num_threads)
{
	size_t cpu_count = SDL_GetCPUCount();
	if (num_threads < cpu_count-1)
	{
		s_activeThreads = num_threads;
	}
	else
	{
		s_activeThreads = cpu_count-1;
	}
	s_running = true;
	s_Workers.resize(s_activeThreads);
	for (size_t i = 0; i < s_Workers.size(); i++)
	{
		s_Workers[i] = std::thread(EC_ThreadFunction, this);
	}
	return 0;
}

std::shared_ptr<EC_Task> EC_ThreadManager::getTask()
{
	std::scoped_lock<std::mutex> lock(s_Lock);

	if (!s_Tasks.empty())
	{
		auto task = s_Tasks.front();
		s_Tasks.pop_front();
		return task;
	}
	return nullptr;
}

std::shared_ptr<EC_Task> EC_ThreadManager::popTaskLocked()
{
	if (s_Tasks.empty())
	{
		return nullptr;
	}
	auto task = s_Tasks.front();
	s_Tasks.pop_front();
	return task;
}

void EC_ThreadManager::addTask(std::shared_ptr<EC_Task> task)
{
	std::scoped_lock<std::mutex> lock(s_Lock);
	s_Tasks.push_back(task);
}


void EC_ThreadManager::executeTasks()
{
	std::scoped_lock<std::mutex> lock(s_Lock);
	s_Flag.notify_all();
}

bool EC_ThreadManager::hasTasks()
{
	std::scoped_lock<std::mutex> lock(s_Lock);
	return !s_Tasks.empty();
}

bool EC_ThreadManager::running()
{
	return s_running;
}

std::condition_variable * EC_ThreadManager::getSignal()
{
	return &s_Flag;
}

std::mutex * EC_ThreadManager::getLock()
{
	return &s_Lock;
}

void EC_ThreadManager::stop()
{
	{
		// Set under the same lock the workers' wait predicate is tested under, so a worker
		// is either yet to check it (and will see false) or already asleep (and gets the
		// notify_all below) - there is no third state where the flag changes unseen and the
		// notification arrives before the worker is waiting.
		std::scoped_lock<std::mutex> lock(s_Lock);
		s_running = false;
	}
	s_Flag.notify_all();

	for (size_t i = 0; i < s_Workers.size(); i++)
	{
		if (s_Workers[i].joinable()) {  // Add this check
			s_Workers[i].join();
		}
	}

	s_Workers.clear();  // Clear the vector after joining
}

EC_ThreadManager::~EC_ThreadManager()
{

}

