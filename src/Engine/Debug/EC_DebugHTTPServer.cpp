#include "Engine/Debug/EC_DebugHTTPServer.h"
// Game.h (and Game_Error's NO_ERROR enumerator) must be included before httplib.h -
// httplib.h drags in <winsock2.h>/<windows.h>, whose NO_ERROR macro (WinError.h) would
// otherwise textually replace Game_Error::NO_ERROR and break the enum.
#include "Game.h"
#include <httplib.h>
#include <chrono>
#include <sstream>
#include "Logging/ECX_Logging.h"
#include "Entity/EC_DOD_EntityManager.h"
#include "Components/EC_DOD_Components.h"

namespace {
    float paramFloat(const httplib::Request& req, const char* name, float def) {
        if (!req.has_param(name)) return def;
        try { return std::stof(req.get_param_value(name)); }
        catch (...) { return def; }
    }

    bool paramBool(const httplib::Request& req, const char* name, bool def) {
        if (!req.has_param(name)) return def;
        std::string v = req.get_param_value(name);
        return v == "1" || v == "true" || v == "True" || v == "TRUE";
    }

    std::string jsonEscape(const std::string& s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
        return out;
    }

    std::string entityName(EntityID entity) {
        auto& manager = EC_DOD_EntityManager::getInstance();
        if (!manager.isAlive(entity) || !manager.hasComponent<EC_DOD_EntityInfo>(entity))
            return "";
        return manager.getComponent<EC_DOD_EntityInfo>(entity).name;
    }

    // Shared by /rayQuery and /coneQuery - normal is always emitted (0,0,0 for cone hits,
    // matching RayQueryHit's own documented convention that it's unused there).
    std::string hitsToJson(const std::vector<RayQueryHit>& hits) {
        std::ostringstream out;
        out << "[";
        for (size_t i = 0; i < hits.size(); i++) {
            const RayQueryHit& hit = hits[i];
            if (i > 0) out << ",";
            out << "{\"entity\":" << hit.entity
                << ",\"name\":\"" << jsonEscape(entityName(hit.entity)) << "\""
                << ",\"position\":{\"x\":" << hit.position.x << ",\"y\":" << hit.position.y << ",\"z\":" << hit.position.z << "}"
                << ",\"normal\":{\"x\":" << hit.normal.x << ",\"y\":" << hit.normal.y << ",\"z\":" << hit.normal.z << "}"
                << ",\"distance\":" << hit.distance << "}";
        }
        out << "]";
        return out.str();
    }

    // Hand-authored rather than generated - the API is small and
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
    },
    "/rayQuery": {
      "get": {
        "summary": "Fire a ray query against the live world and draw it as a debug ray",
        "description": "Same query EC_GameAPI's Lua rayQuery()/showDebugRay() bindings expose to click-driven tests (see RayConeClickTest.lua) - exposed over HTTP so a ray can be fired and its trace screenshotted without driving the game window directly. Always also pushes the debug ray visualization (same as showDebugRay), so a follow-up GET /screenshot shows the traced line.",
        "parameters": [
          { "name": "ox", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Ray origin X." },
          { "name": "oy", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Ray origin Y." },
          { "name": "oz", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Ray origin Z." },
          { "name": "dx", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Ray direction X (need not be normalized)." },
          { "name": "dy", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Ray direction Y." },
          { "name": "dz", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Ray direction Z." },
          { "name": "maxDistance", "in": "query", "required": false, "schema": { "type": "number", "default": 100 }, "description": "Maximum ray distance." },
          { "name": "firstHitOnly", "in": "query", "required": false, "schema": { "type": "boolean", "default": false }, "description": "Stop at the nearest hit instead of returning every entity the ray intersects." }
        ],
        "responses": {
          "200": {
            "description": "Array of hits, nearest not guaranteed first unless firstHitOnly.",
            "content": { "application/json": { "schema": { "type": "array", "items": { "type": "object" } } } }
          },
          "400": {
            "description": "Missing a required parameter, or no live EC_Game to query against.",
            "content": { "text/plain": { "schema": { "type": "string" } } }
          }
        }
      }
    },
    "/coneQuery": {
      "get": {
        "summary": "Fire a cone query against the live world and draw it as a debug cone",
        "description": "Same query EC_GameAPI's Lua coneQuery()/showDebugCone() bindings expose to click-driven tests. Always also pushes the debug cone visualization, so a follow-up GET /screenshot shows the traced cone.",
        "parameters": [
          { "name": "ax", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Cone apex X." },
          { "name": "ay", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Cone apex Y." },
          { "name": "az", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Cone apex Z." },
          { "name": "dx", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Cone axis direction X (need not be normalized)." },
          { "name": "dy", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Cone axis direction Y." },
          { "name": "dz", "in": "query", "required": true, "schema": { "type": "number" }, "description": "Cone axis direction Z." },
          { "name": "halfAngleDegrees", "in": "query", "required": false, "schema": { "type": "number", "default": 20 }, "description": "Cone half-angle in degrees." },
          { "name": "maxDistance", "in": "query", "required": false, "schema": { "type": "number", "default": 100 }, "description": "Maximum cone distance." },
          { "name": "castsShadowOnly", "in": "query", "required": false, "schema": { "type": "boolean", "default": true }, "description": "Restrict to castsShadow == true geometry, matching the Lua binding's default." },
          { "name": "checkOcclusion", "in": "query", "required": false, "schema": { "type": "boolean", "default": false }, "description": "Additionally require unobstructed line-of-sight to the apex." }
        ],
        "responses": {
          "200": {
            "description": "Array of hits (containment only - position is each entity's own world position, not a surface point; normal is unused, always zero).",
            "content": { "application/json": { "schema": { "type": "array", "items": { "type": "object" } } } }
          },
          "400": {
            "description": "Missing a required parameter, or no live EC_Game to query against.",
            "content": { "text/plain": { "schema": { "type": "string" } } }
          }
        }
      }
    },
    "/regenerateTerrain": {
      "get": {
        "summary": "Re-run the terrain generation script and regenerate every voxel chunk",
        "description": "Re-runs data/scripts/LUA/TerrainGeneration.lua and re-schedules every existing chunk against the resulting shape, live - no engine restart needed to see a script edit's effect. Only requests regeneration (flips a flag checked once per frame); the actual work happens on the main thread on its next tick, so chunks update over the following few frames rather than instantly.",
        "responses": {
          "200": {
            "description": "Regeneration requested.",
            "content": { "text/plain": { "schema": { "type": "string" } } }
          },
          "400": {
            "description": "No live EC_Game to regenerate against.",
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

EC_DebugHTTPServer::EC_DebugHTTPServer(std::string host, int port, EC_Game* game)
    : m_Host(std::move(host))
    , m_Port(port)
    , m_Game(game)
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

    // Unlike /screenshot, no GL work and no rendezvous needed: EC_Game::queryRay and
    // showDebugRay each end up calling into a mutex-guarded queue/snapshot (see this
    // constructor's own parameter comment) that's already designed for cross-thread
    // access, so this handler can call straight into EC_Game from its own httplib worker
    // thread.
    m_Server->Get("/rayQuery", [this](const httplib::Request& req, httplib::Response& res) {
        if (!m_Game) {
            res.status = 400;
            res.set_content("no live EC_Game to query against", "text/plain");
            return;
        }
        if (!req.has_param("ox") || !req.has_param("oy") || !req.has_param("oz") ||
            !req.has_param("dx") || !req.has_param("dy") || !req.has_param("dz")) {
            res.status = 400;
            res.set_content("missing required parameter(s): ox, oy, oz, dx, dy, dz", "text/plain");
            return;
        }

        glm::vec3 origin(paramFloat(req, "ox", 0.0f), paramFloat(req, "oy", 0.0f), paramFloat(req, "oz", 0.0f));
        glm::vec3 direction(paramFloat(req, "dx", 0.0f), paramFloat(req, "dy", 0.0f), paramFloat(req, "dz", 0.0f));
        float maxDistance = paramFloat(req, "maxDistance", 100.0f);
        bool firstHitOnly = paramBool(req, "firstHitOnly", false);

        m_Game->showDebugRay(origin, direction, maxDistance);
        std::vector<RayQueryHit> hits = m_Game->queryRay(origin, direction, maxDistance, firstHitOnly);
        res.set_content(hitsToJson(hits), "application/json");
    });

    m_Server->Get("/coneQuery", [this](const httplib::Request& req, httplib::Response& res) {
        if (!m_Game) {
            res.status = 400;
            res.set_content("no live EC_Game to query against", "text/plain");
            return;
        }
        if (!req.has_param("ax") || !req.has_param("ay") || !req.has_param("az") ||
            !req.has_param("dx") || !req.has_param("dy") || !req.has_param("dz")) {
            res.status = 400;
            res.set_content("missing required parameter(s): ax, ay, az, dx, dy, dz", "text/plain");
            return;
        }

        glm::vec3 apex(paramFloat(req, "ax", 0.0f), paramFloat(req, "ay", 0.0f), paramFloat(req, "az", 0.0f));
        glm::vec3 direction(paramFloat(req, "dx", 0.0f), paramFloat(req, "dy", 0.0f), paramFloat(req, "dz", 0.0f));
        float halfAngleDegrees = paramFloat(req, "halfAngleDegrees", 20.0f);
        float maxDistance = paramFloat(req, "maxDistance", 100.0f);
        bool castsShadowOnly = paramBool(req, "castsShadowOnly", true);
        bool checkOcclusion = paramBool(req, "checkOcclusion", false);

        m_Game->showDebugCone(apex, direction, halfAngleDegrees, maxDistance);
        std::vector<RayQueryHit> hits = m_Game->queryCone(apex, direction, halfAngleDegrees, maxDistance, castsShadowOnly, checkOcclusion);
        res.set_content(hitsToJson(hits), "application/json");
    });

    // Re-runs data/scripts/LUA/TerrainGeneration.lua and re-schedules every voxel chunk
    // against the new shape - lets an author edit the script and see the result without a
    // full engine restart. Safe from this handler's own httplib worker thread: it only
    // flips an atomic flag (EC_VoxelChunkSystem::requestRegenerate()) - the actual
    // regeneration work happens on the main thread on its next update() tick, same
    // reasoning as GET /rayQuery not needing a rendezvous.
    m_Server->Get("/regenerateTerrain", [this](const httplib::Request&, httplib::Response& res) {
        if (!m_Game) {
            res.status = 400;
            res.set_content("no live EC_Game to regenerate against", "text/plain");
            return;
        }
        m_Game->regenerateTerrain();
        res.set_content("regeneration requested", "text/plain");
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
