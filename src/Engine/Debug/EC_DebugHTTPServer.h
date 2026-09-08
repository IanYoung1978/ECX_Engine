#pragma once
#include <memory>
#include <string>
#include "TaskManager/EC_Task.h"

namespace httplib { class Server; }

// A minimal, dev-build-only, read-only HTTP debug surface - Issue #87's first slice,
// currently just GET /log. Shaped like EC_VoxelChunkWorker: a long-lived EC_Task added
// once to the shared thread pool, whose execute() blocks for the life of the program
// until shutdown() unblocks it. Unlike EC_VoxelChunkWorker's condvar-based wait, this one
// blocks inside httplib::Server::listen() - a real OS-level accept() loop, which a
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

private:
    std::string m_Host;
    int m_Port;
    // Constructed in the constructor (main thread, before this task is ever added to the
    // pool) rather than lazily inside execute() - httplib::Server's constructor touches
    // no sockets/threads (only listen() does), and constructing it early means shutdown()
    // can never race a not-yet-assigned pointer if it's called before execute() has
    // actually started running on its worker thread.
    std::unique_ptr<httplib::Server> m_Server;
};
