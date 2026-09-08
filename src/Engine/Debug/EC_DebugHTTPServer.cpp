#include "Engine/Debug/EC_DebugHTTPServer.h"
#include <httplib.h>
#include <chrono>
#include "Logging/ECX_Logging.h"

namespace {
    // Hand-authored rather than generated - the API is small (3 read-only routes) and
    // changes rarely enough that keeping this in sync by hand alongside route changes is
    // the pragmatic choice over pulling in a spec-generation library for a dev tool.
    const std::string kOpenApiSpec = R"JSON({
  "openapi": "3.0.3",
  "info": {
    "title": "ECX_Engine Debug HTTP API",
    "description": "Dev-build-only, read-only debug interface into a running ECX_Engine instance (Issue #87). Localhost-bound, no authentication - never enabled in a retail build. See GET /docs for an interactive viewer of this spec.",
    "version": "1.0.0"
  },
  "paths": {
    "/log": {
      "get": {
        "summary": "Recent engine log lines",
        "parameters": [
          {
            "name": "count",
            "in": "query",
            "required": false,
            "schema": { "type": "integer", "default": 50 },
            "description": "Number of recent log lines to return, most recent last. Capped by the engine's own retained log history (200 lines)."
          }
        ],
        "responses": {
          "200": {
            "description": "Log lines, one per line.",
            "content": { "text/plain": { "schema": { "type": "string" } } }
          }
        }
      }
    },
    "/screenshot": {
      "get": {
        "summary": "Capture the running engine's current frame",
        "parameters": [
          {
            "name": "target",
            "in": "query",
            "required": false,
            "schema": {
              "type": "string",
              "enum": ["final", "albedo", "normal", "depth"],
              "default": "final"
            },
            "description": "Which buffer to capture. 'final' (or omitted) is the fully composited frame - scene, skybox, debug overlay, and UI, exactly what's on screen. 'albedo'/'normal'/'depth' are individual G-buffer attachments, each with its own visualization: albedo is gamma-encoded linear colour, normal is world-space [-1,1] remapped to [0,1] per channel, depth is raw non-linear NDC depth (not linearized - nearby geometry reads much darker than its true distance would suggest)."
          }
        ],
        "responses": {
          "200": {
            "description": "PNG image of the requested buffer.",
            "content": { "image/png": { "schema": { "type": "string", "format": "binary" } } }
          },
          "503": {
            "description": "Capture failed or timed out (e.g. the main loop is stalled or paused).",
            "content": { "text/plain": { "schema": { "type": "string" } } }
          }
        }
      }
    }
  }
})JSON";

    const std::string kSwaggerUIPage = R"HTML(<!DOCTYPE html>
<html>
<head>
  <title>ECX_Engine Debug API</title>
  <link rel="stylesheet" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css">
</head>
<body>
  <div id="swagger-ui"></div>
  <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js"></script>
  <script>
    window.onload = () => {
      SwaggerUIBundle({ url: '/openapi.json', dom_id: '#swagger-ui' });
    };
  </script>
</body>
</html>
)HTML";
}

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

    m_Server->Get("/openapi.json", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(kOpenApiSpec, "application/json");
    });

    m_Server->Get("/docs", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(kSwaggerUIPage, "text/html");
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
