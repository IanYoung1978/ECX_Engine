#pragma once
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "TaskManager/EC_Task.h"

namespace httplib { class Server; }

// A minimal, dev-build-only, read-only HTTP debug surface - Issue #87's first two slices,
// currently GET /log and GET /screenshot. Shaped like EC_VoxelChunkWorker: a long-lived
// EC_Task added once to the shared thread pool, whose execute() blocks for the life of the
// program until shutdown() unblocks it. Unlike EC_VoxelChunkWorker's condvar-based wait,
// this one blocks inside httplib::Server::listen() - a real OS-level accept() loop, which a
// condition_variable notify (the pattern every other background task in this codebase
// uses to unblock) cannot reach. shutdown() calls Server::stop(), which httplib
// implements by closing the listening socket - the mechanism that actually unblocks it.
class EC_DebugHTTPServer : public EC_Task {
public:
    EC_DebugHTTPServer(std::string host, int port);
    ~EC_DebugHTTPServer() override;

    void execute() override;

    // Must be called before EC_ThreadManager::stop() joins the shared pool - the pool
    // join would otherwise deadlock waiting on a thread parked in listen() that never
    // observes the pool-level shutdown signal on its own (same requirement, same reason,
    // as EC_VoxelChunkSystem::shutdown() - see its own comment).
    void shutdown();

    // GET /screenshot needs GL work (glReadPixels) that's only valid on the GL/main
    // thread, but the handler runs on one of httplib's own worker threads - this is the
    // single-slot rendezvous that bridges the two. Called from the HTTP handler thread:
    // blocks until the main thread services the request (hasPendingCapture/
    // completeCapture below) or ~5s elapses. A mutex-held single slot rather than a real
    // queue - this is a low-traffic dev tool, not perf-sensitive, so concurrent requests
    // simply serialize behind each other rather than needing real batching.
    std::vector<unsigned char> requestCapture();

    // Called from the main thread once per frame (see EC_Game::update()). Returns true
    // if an HTTP thread is currently blocked in requestCapture() waiting on a result.
    bool hasPendingCapture();
    // Called from the main thread right after producing (or failing to produce) the
    // capture hasPendingCapture() just reported - wakes the waiting HTTP thread.
    void completeCapture(std::vector<unsigned char> pngBytes, bool ok);

private:
    std::string m_Host;
    int m_Port;
    // Constructed in the constructor (main thread, before this task is ever added to the
    // pool) rather than lazily inside execute() - httplib::Server's constructor touches
    // no sockets/threads (only listen() does), and constructing it early means shutdown()
    // can never race a not-yet-assigned pointer if it's called before execute() has
    // actually started running on its worker thread.
    std::unique_ptr<httplib::Server> m_Server;

    std::mutex m_CaptureMutex;
    std::condition_variable m_CaptureCV;
    bool m_CapturePending = false;
    bool m_CaptureResultReady = false;
    bool m_CaptureOk = false;
    std::vector<unsigned char> m_CaptureResult;
};
