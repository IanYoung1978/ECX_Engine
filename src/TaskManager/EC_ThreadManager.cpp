#include "EC_ThreadManager.h"
#include <SDL_cpuinfo.h>
#include <string>

int EC_ThreadFunction(EC_ThreadManager* manager)
{
	while (manager->running())
	{
		while (!manager->hasTasks())
		{
			manager->getSignal()->wait(std::unique_lock<std::mutex>(*(manager->getLock())));
			if (!manager->running())
			{
				break;
			}
		}
		auto task = manager->getTask();
		if (task != nullptr)
		{
			task->execute();
			task = manager->getTask();
		}
	}
	return 0;
}

std::vector<std::thread> EC_ThreadManager::s_Workers;
std::deque<std::shared_ptr<EC_Task>> EC_ThreadManager::s_Tasks;
std::mutex EC_ThreadManager::s_Lock;
std::condition_variable EC_ThreadManager::s_Flag;
bool EC_ThreadManager::s_running;
int EC_ThreadManager::s_activeThreads;

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
	s_running = false;
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

