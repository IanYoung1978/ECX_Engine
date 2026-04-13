#include "EC_DOD_LoadingWorker.h"
#include "Entity/EC_DOD_EntityFactory.h"
#include "SceneManager/EC_GameScene.h"
#include "Components/EC_DOD_Components.h"
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

void EC_DOD_LoadingWorker::scheduleEntity(const std::string& filename, EC_GameScene* scene) {
    {
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        std::lock_guard<std::mutex> progressLock(m_ProgressMutex);
        m_LoadQueue.push_back({ LoadTask::Type::Entity, filename, scene });
        if (m_Loading)
            m_TotalTasks++;
    }
    m_QueueCV.notify_one();
}

void EC_DOD_LoadingWorker::scheduleScene(const std::string& filename, EC_GameScene& scene) {
    {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "Scheduling scene: " + filename,
            LOGGING::LogLevel::INFORMATION);
        std::lock_guard<std::mutex> queueLock(m_QueueMutex);
        std::lock_guard<std::mutex> progressLock(m_ProgressMutex);
        m_LoadQueue.push_back({ LoadTask::Type::Scene, filename, &scene });
        if (m_Loading)
            m_TotalTasks++;
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
    if (m_Loading)
        m_QueueCV.notify_one();
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
    if (!m_ReadyToFinalize.load() || m_Finalized.load())
        return;

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Finalizing resources on main thread...",
        LOGGING::LogLevel::INFORMATION);

    m_Factory.performPostLoadActions();
    m_Finalized = true;
    m_ReadyToFinalize = false;

    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "Finalization complete",
        LOGGING::LogLevel::INFORMATION);
}

void EC_DOD_LoadingWorker::execute() {
    while (m_Running) {
        LoadTask task;
        bool hasTask = false;

        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_QueueCV.wait(lock, [this] {
                return !m_Running || (!m_LoadQueue.empty() && m_Loading);
                });

            if (!m_Running) break;

            if (!m_LoadQueue.empty() && m_Loading) {
                task = m_LoadQueue.front();
                m_LoadQueue.pop_front();
                hasTask = true;
            }
        }

        if (!hasTask) continue;

        bool success = false;

        if (task.type == LoadTask::Type::Entity && task.scene)
            success = loadEntityFile(task.filename, task.scene);
        else if (task.type == LoadTask::Type::Scene && task.scene)
            success = loadSceneFile(task.filename, *task.scene);

        {
            std::lock_guard<std::mutex> progressLock(m_ProgressMutex);
            m_CompletedTasks++;

            if (success)
                LOGGING::ECX_Logger::GetInstance()->LogMessage(
                    "Loaded: " + task.filename + " (" +
                    std::to_string(m_CompletedTasks) + "/" +
                    std::to_string(m_TotalTasks) + ")",
                    LOGGING::LogLevel::INFORMATION);
            else
                LOGGING::ECX_Logger::GetInstance()->LogMessage(
                    "Failed to load: " + task.filename,
                    LOGGING::LogLevel::SEVERE);

            std::lock_guard<std::mutex> queueLock(m_QueueMutex);
            if (m_CompletedTasks >= m_TotalTasks && m_LoadQueue.empty()) {
                m_Loading = false;
                m_ReadyToFinalize = true;
                LOGGING::ECX_Logger::GetInstance()->LogMessage(
                    "Loading complete: " + std::to_string(m_CompletedTasks) +
                    " tasks completed. Ready for finalization.",
                    LOGGING::LogLevel::INFORMATION);
            }
        }
    }
}

bool EC_DOD_LoadingWorker::loadEntityFile(const std::string& filename, EC_GameScene* scene) {
    TiXmlDocument doc(filename.c_str());
    if (!doc.LoadFile()) return false;
    TiXmlElement* root = doc.FirstChildElement();
    if (!root) return false;
    parseEntity(root, *scene);
    return true;
}

bool EC_DOD_LoadingWorker::loadSceneFile(const std::string& filename, EC_GameScene& scene) {
    TiXmlDocument doc(filename.c_str());
    if (!doc.LoadFile()) return false;
    TiXmlElement* root = doc.FirstChildElement();
    if (!root || strcmp(root->Value(), "Scene") != 0) return false;

    TiXmlElement* manifest = root->FirstChildElement("Manifest");
    if (manifest)
        EC_DOD_EntityFactory::parseManifest(manifest);

    parseSceneEntities(root, scene);
    scene.setLoaded(true);
    return true;
}

EntityID EC_DOD_LoadingWorker::parseEntity(TiXmlElement* element, EC_GameScene& scene) {
    EntityID entity = m_Factory.constructEntity(*element);
    if (entity != INVALID_ENTITY) {
        scene.addEntity(entity);
        auto& manager = EC_DOD_EntityManager::getInstance();
        if (manager.hasComponent<EC_DOD_Camera>(entity))
            scene.addCamera(entity);
        if (manager.hasComponent<EC_DOD_Light>(entity))
            scene.addLight(entity);
    }
    return entity;
}

void EC_DOD_LoadingWorker::parseSceneEntities(TiXmlElement* sceneRoot, EC_GameScene& scene) {
    TiXmlElement* elem = sceneRoot->FirstChildElement();
    while (elem) {
        if (strcmp(elem->Value(), "Entity") == 0) {
            TiXmlElement* child = elem->FirstChildElement();
            if (child && strcmp(child->Value(), "Filename") == 0)
                loadEntityFile(child->GetText(), &scene);
            else
                parseEntity(elem, scene);
        }
        else if (strcmp(elem->Value(), "Manifest") == 0)
        {
            // ignored - already parsed
        }
        elem = elem->NextSiblingElement();
    }
}