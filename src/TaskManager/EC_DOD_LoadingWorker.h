#pragma once
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "Entity/EC_DOD_EntityManager.h"
#include "TaskManager/EC_Task.h"
#include "Entity/EC_DOD_EntityFactory.h"

class TiXmlElement;

class EC_DOD_LoadingWorker : public EC_Task {
public:
    EC_DOD_LoadingWorker();
    ~EC_DOD_LoadingWorker();

    void scheduleEntity(const std::string& filename);
    void scheduleScene(const std::string& filename);

    void start();
    void abort();
    void shutdown();

    float getProgress() const;
    bool isLoading() const;

    bool needsFinalization() const;
    void finalizeOnMainThread();

    // Inherited from EC_Task
    void execute() override;

private:
    struct LoadTask {
        enum class Type {
            Entity,
            Scene
        };
        Type type;
        std::string filename;
    };

    bool loadEntityFile(const std::string& filename);
    bool loadSceneFile(const std::string& filename);
    EntityID parseEntity(TiXmlElement* element);
    void parseSceneEntities(TiXmlElement* sceneRoot);

    std::atomic<bool> m_Running;
    std::atomic<bool> m_Loading;
    std::atomic<bool> m_ReadyToFinalize;
    std::atomic<bool> m_Finalized;

    mutable std::mutex m_QueueMutex;
    std::condition_variable m_QueueCV;
    std::deque<LoadTask> m_LoadQueue;

    mutable std::mutex m_ProgressMutex;
    size_t m_TotalTasks;
    size_t m_CompletedTasks;
    EC_DOD_EntityFactory m_Factory;
};