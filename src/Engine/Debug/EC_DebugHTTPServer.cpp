#include "Engine/Debug/EC_DebugHTTPServer.h"
#include <httplib.h>
#include <chrono>
#include "Logging/ECX_Logging.h"

EC_DebugHTTPServer::EC_DebugHTTPServer(std::string host, int port)
    : m_Host(std::move(host))
    , m_Port(port)
    , m_Server(std::make_unique<httplib::Server>())
{
    // GetRecentPlainLogs() is already std::mutex-guarded internally (see ECX_Logging.h) -
    // safe to call directly from this handler's own thread (httplib's worker thread, not
    // the main/render thread), no new locking needed for this route.
    m_Server->Get("/log", [](const httplib::Request& req, httplib::Response& res) {
        size_t count = 50;
        if (req.has_param("count")) {
            try {
                count = static_cast<size_t>(std::stoul(req.get_param_value("count")));
            }
            catch (...) {
                count = 50;
            }
        }

        auto lines = LOGGING::ECX_Logger::GetInstance()->GetRecentPlainLogs(count);
        std::string body;
        for (const auto& line : lines) {
            body += line;
            body += '\n';
        }
        res.set_content(body, "text/plain");
    });

    // Unlike /log, this needs real GL work (glReadPixels) that's only valid on the
    // GL/main thread - requestCapture() bridges this handler's own thread (one of
    // httplib's worker threads) to the main thread via the mutex+condvar rendezvous
    // below, serviced once per frame from EC_Game::update().
    m_Server->Get("/screenshot", [this](const httplib::Request& req, httplib::Response& res) {
        std::string target = req.has_param("target") ? req.get_param_value("target") : "";
        std::vector<unsigned char> png = requestCapture(target);
        if (png.empty()) {
            res.status = 503;
            res.set_content("screenshot capture failed or timed out", "text/plain");
            return;
        }
        res.set_content(reinterpret_cast<const char*>(png.data()), png.size(), "image/png");
    });
}

EC_DebugHTTPServer::~EC_DebugHTTPServer() {
    shutdown();
}

void EC_DebugHTTPServer::execute() {
    LOGGING::ECX_Logger::GetInstance()->LogMessage(
        "EC_DebugHTTPServer: listening on " + m_Host + ":" + std::to_string(m_Port),
        LOGGING::LogLevel::INFORMATION);

    if (!m_Server->listen(m_Host, m_Port)) {
        LOGGING::ECX_Logger::GetInstance()->LogMessage(
            "EC_DebugHTTPServer: failed to bind " + m_Host + ":" + std::to_string(m_Port),
            LOGGING::LogLevel::SEVERE);
    }
}

void EC_DebugHTTPServer::shutdown() {
    if (m_Server) m_Server->stop();
}

std::vector<unsigned char> EC_DebugHTTPServer::requestCapture(const std::string& target) {
    std::unique_lock<std::mutex> lock(m_CaptureMutex);
    // Serializes concurrent requesters behind each other rather than needing a real
    // multi-slot queue - see the class comment for why that's fine for this tool.
    m_CaptureCV.wait(lock, [this] { return !m_CapturePending; });

    m_CapturePending = true;
    m_CaptureResultReady = false;
    m_CaptureTarget = target;
    m_CaptureResult.clear();

    bool completed = m_CaptureCV.wait_for(lock, std::chrono::seconds(5),
        [this] { return m_CaptureResultReady; });

    std::vector<unsigned char> result;
    if (completed && m_CaptureOk) {
        result = std::move(m_CaptureResult);
    }

    m_CapturePending = false;
    lock.unlock();
    m_CaptureCV.notify_all();
    return result;
}

bool EC_DebugHTTPServer::hasPendingCapture(std::string& outTarget) {
    std::lock_guard<std::mutex> lock(m_CaptureMutex);
    if (!m_CapturePending || m_CaptureResultReady) return false;
    outTarget = m_CaptureTarget;
    return true;
}

void EC_DebugHTTPServer::completeCapture(std::vector<unsigned char> pngBytes, bool ok) {
    {
        std::lock_guard<std::mutex> lock(m_CaptureMutex);
        m_CaptureResult = std::move(pngBytes);
        m_CaptureOk = ok;
        m_CaptureResultReady = true;
    }
    m_CaptureCV.notify_all();
}
