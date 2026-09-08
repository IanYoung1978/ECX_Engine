#include "Engine/Debug/EC_DebugHTTPServer.h"
#include <httplib.h>
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
