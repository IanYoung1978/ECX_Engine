#pragma once
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "TaskManager/EC_Task.h"

namespace httplib { class Server; }
class EC_Game;

// Dev-build-only, read-only HTTP debug surface, run as one long-lived task on the shared pool.
// execute() blocks in httplib's listen(); shutdown() calls Server::stop(), which closes the listening
// socket (a condition variable can't unblock an accept() loop).
class EC_DebugHTTPServer : public EC_Task {
public:
    // `game` may be nullptr (disables /rayQuery and /coneQuery with a 503) so tools without a live
    // EC_Game can still construct this. The ray/cone queries and debug draws are mutex-guarded, so they
    // are safe from the httplib worker thread, unlike /screenshot's GL work.
    EC_DebugHTTPServer(std::string host, int port, EC_Game* game = nullptr);
    ~EC_DebugHTTPServer() override;

    void execute() override;

    // Must be called before EC_ThreadManager::stop() joins the shared pool - the pool
    // join would otherwise deadlock waiting on a thread parked in listen() that never
    // observes the pool-level shutdown signal on its own (same requirement, same reason,
    // as EC_VoxelChunkSystem::shutdown() - see its own comment).
    void shutdown();

    // GET /screenshot[?target=<name>] needs GL calls that are only valid on the main thread, so the
    // handler thread hands the request over through this single mutex-held slot and blocks until the main
    // thread services it (hasPendingCapture/completeCapture) or ~5s pass; concurrent requests serialize.
    // `target` goes to Renderer::captureFrame ("", "final", "albedo", "normal", "depth").
    std::vector<unsigned char> requestCapture(const std::string& target);

    // Called from the main thread once per frame (see EC_Game::update()). Returns true
    // (and fills outTarget) if an HTTP thread is currently blocked in requestCapture()
    // waiting on a result.
    bool hasPendingCapture(std::string& outTarget);
    // Called from the main thread right after producing (or failing to produce) the
    // capture hasPendingCapture() just reported - wakes the waiting HTTP thread.
    void completeCapture(std::vector<unsigned char> pngBytes, bool ok);

private:
    std::string m_Host;
    int m_Port;
    EC_Game* m_Game;
    // Constructed up front on the main thread so shutdown() can't race an unassigned pointer before
    // execute() has started.
    std::unique_ptr<httplib::Server> m_Server;

    std::mutex m_CaptureMutex;
    std::condition_variable m_CaptureCV;
    bool m_CapturePending = false;
    bool m_CaptureResultReady = false;
    bool m_CaptureOk = false;
    std::string m_CaptureTarget;
    std::vector<unsigned char> m_CaptureResult;
};
