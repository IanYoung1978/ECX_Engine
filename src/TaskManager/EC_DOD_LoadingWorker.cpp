#include "EC_DOD_LoadingWorker.h"
#include "Entity/EC_DOD_EntityFactory.h"
#include "xml/tinyxml.h"
#include "Logging/ECX_Logging.h"

EC_DOD_LoadingWorker::EC_DOD_LoadingWorker()
    : m_Running(true)
    , m_Loading(false)
    , m_TotalTasks(0)
    , m_CompletedTasks(0)
    , m_ReadyToFinalize(false)
    , m_Finalized(false)
{
}

EC_DOD_LoadingWorker::~EC_DOD_LoadingWorker() {
    shutdown();
}

void EC_DOD_LoadingWorker::scheduleEntity(const std::string& filename) {
    {
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        std::lock_guard<std::mutex> progressLock(m_ProgressMutex);

        m_LoadQueue.push_back({ LoadTask::Type::Entity, filename });

        // Increment total tasks if we're currently loading
        if (m_Loading) {
            m_TotalTasks++;
        }
    }
    m_QueueCV.notify_one();
}

void EC_DOD_LoadingWorker::scheduleScene(const std::string& filename) {
    {
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        std::lock_guard<std::mutex> progressLock(m_ProgressMutex);

        m_LoadQueue.push_back({ LoadTask::Type::Scene, filename });

        // Increment total tasks if we're currently loading
        if (m_Loading) {
            m_TotalTasks++;
        }
    }
    m_QueueCV.notify_one();
}

void EC_DOD_LoadingWorker::start() {
    {
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        std::lock_guard<std::mutex> progressLock(m_ProgressMutex);

        m_TotalTasks = m_LoadQueue.size();
        m_CompletedTasks = 0;
        m_Loading = (m_TotalTasks > 0);
        m_ReadyToFinalize = false;
        m_Finalized = false;
    }

    if (m_Loading) {
        m_QueueCV.notify_one();
    }
}

void EC_DOD_LoadingWorker::abort() {
    {
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        std::lock_guard<std::mutex> progressLock(m_ProgressMutex);

        m_LoadQueue.clear();
        m_Loading = false;
        m_CompletedTasks = 0;
        m_TotalTasks = 0;
        m_ReadyToFinalize = false;
        m_Finalized = false;
    }
    m_QueueCV.notify_one();
}

void EC_DOD_LoadingWorker::shutdown() {
    m_Running = false;
    m_Loading = false;
    {
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        m_LoadQueue.clear();
	}
    m_QueueCV.notify_one();
}

float EC_DOD_LoadingWorker::getProgress() const {
    std::lock_guard<std::mutex> lock(m_ProgressMutex);
    if (m_TotalTasks == 0) return 100.0f;
    return (100.0f * static_cast<float>(m_CompletedTasks) / static_cast<float>(m_TotalTasks));
}

bool EC_DOD_LoadingWorker::isLoading() const {
    return m_Loading.load();
}

bool EC_DOD_LoadingWorker::needsFinalization() const {
    return m_ReadyToFinalize.load() && !m_Finalized.load();
}

void EC_DOD_LoadingWorker::finalizeOnMainThread() {
    if (!m_ReadyToFinalize.load() || m_Finalized.load()) {
        return;  // Already finalized or not ready yet
    }

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Finalizing resources on main thread...",
        LOGGING::LogLevel::INFORMATION
    );
	m_Factory.performPostLoadActions();
    m_Finalized = true;
    m_ReadyToFinalize = false;

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Finalization complete",
        LOGGING::LogLevel::INFORMATION
    );
}

void EC_DOD_LoadingWorker::execute() {
    while (m_Running) {
        LoadTask task;
        bool hasTask = false;

        // Wait for work or shutdown
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);

            // Wait until we have work or need to shutdown
            m_QueueCV.wait(lock, [this] {
                return !m_Running || (!m_LoadQueue.empty() && m_Loading);
                });

            if (!m_Running) {
                break;
            }

            if (!m_LoadQueue.empty() && m_Loading) {
                task = m_LoadQueue.front();
                m_LoadQueue.pop_front();
                hasTask = true;
            }
        }

        if (!hasTask) {
            continue;
        }

        bool success = false;

        if (task.type == LoadTask::Type::Entity) {
            success = loadEntityFile(task.filename);
        }
        else if (task.type == LoadTask::Type::Scene) {
            success = loadSceneFile(task.filename);
        }

        // Update progress
        {
            std::lock_guard<std::mutex> progressLock(m_ProgressMutex);
            m_CompletedTasks++;

            if (success) {
                LOGGING::ECX_Logger::GetInstance()->LogMessage(
                    "Loaded: " + task.filename + " (" +
                    std::to_string(m_CompletedTasks) + "/" +
                    std::to_string(m_TotalTasks) + ")",
                    LOGGING::LogLevel::INFORMATION
                );
            }
            else {
                LOGGING::ECX_Logger::GetInstance()->LogMessage(
                    "Failed to load: " + task.filename,
                    LOGGING::LogLevel::SEVERE
                );
            }

            // Check if we're done - also check the queue
            std::lock_guard<std::mutex> queueLock(m_QueueMutex);
            if (m_CompletedTasks >= m_TotalTasks && m_LoadQueue.empty()) {
                m_Loading = false;
                m_ReadyToFinalize = true;  // Signal main thread

                LOGGING::ECX_Logger::GetInstance()->LogMessage(
                    "Loading complete: " + std::to_string(m_CompletedTasks) +
                    " tasks completed. Ready for finalization.",
                    LOGGING::LogLevel::INFORMATION
                );
            }
        }
    }
}

bool EC_DOD_LoadingWorker::loadEntityFile(const std::string& filename) {
    TiXmlDocument doc(filename.c_str());

    if (!doc.LoadFile()) {
        return false;
    }

    TiXmlElement* root = doc.FirstChildElement();
    if (!root) {
        return false;
    }

    EntityID entity = parseEntity(root);
    return (entity != INVALID_ENTITY);
}

bool EC_DOD_LoadingWorker::loadSceneFile(const std::string& filename) {
    TiXmlDocument doc(filename.c_str());

    if (!doc.LoadFile()) {
        return false;
    }

    TiXmlElement* root = doc.FirstChildElement();
    if (!root || strcmp(root->Value(), "Scene") != 0) {
        return false;
    }

    parseSceneEntities(root);
    return true;
}

EntityID EC_DOD_LoadingWorker::parseEntity(TiXmlElement* element) {
    return m_Factory.constructEntity(*element);
}

void EC_DOD_LoadingWorker::parseSceneEntities(TiXmlElement* sceneRoot) {
    TiXmlElement* elem = sceneRoot->FirstChildElement();

    while (elem) {
        if (strcmp(elem->Value(), "Entity") == 0) {
            TiXmlElement* child = elem->FirstChildElement();

            if (child && strcmp(child->Value(), "Filename") == 0) {
                // This schedules a new task and increments m_TotalTasks
                scheduleEntity(child->GetText());
            }
            else {
                // Inline entity - parse directly
                parseEntity(elem);
            }
        }

        elem = elem->NextSiblingElement();
    }
}