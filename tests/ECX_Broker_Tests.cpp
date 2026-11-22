// Unit tests for ECXEventBroker and ECXCommandBroker: delivery order, dropping messages that
// have no subscriber, deferral of messages published during a flush, and publishing from other
// threads while the main thread flushes.
#include <catch2/catch_test_macros.hpp>
#include "Messaging/ECXEventBroker.h"
#include "Messaging/IEventListener.h"
#include "Messaging/ECXCommandBroker.h"
#include "Messaging/ICommandListener.h"
#include <any>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace {

// Runs fn on a detached thread and reports whether it finished within `limit`, so a flush() that
// never returns fails the test instead of hanging the whole run. Everything fn touches must be
// kept alive by fn itself (shared_ptr captures), since a timed-out thread keeps running.
template <class F>
bool completesWithin(std::chrono::milliseconds limit, F fn) {
    auto done = std::make_shared<std::promise<void>>();
    auto finished = done->get_future();
    std::thread([done, fn]() mutable { fn(); done->set_value(); }).detach();
    return finished.wait_for(limit) == std::future_status::ready;
}

ECXEvent makeEvent(ECXEventType type, int id) {
    ECXEvent e;
    e.type = type;
    e.args[0] = id;
    return e;
}

ECXCommand makeCommand(ECXCommandType type, int id) {
    ECXCommand c;
    c.type = type;
    c.args[0] = id;
    return c;
}

struct RecordingEventListener : IEventListener {
    std::vector<int> ids;
    void receive(ECXEvent& e) override { ids.push_back(std::any_cast<int>(e.args[0])); }
};

struct RecordingCommandListener : ICommandListener {
    std::vector<int> ids;
    void receive(ECXCommand& c) override { ids.push_back(std::any_cast<int>(c.args[0])); }
};

// Publishes a second event the first time it receives one - to check that a publish from inside
// a listener neither deadlocks nor is delivered within the same flush.
struct RepublishingListener : IEventListener {
    ECXEventBroker& broker;
    std::vector<int> ids;
    explicit RepublishingListener(ECXEventBroker& b) : broker(b) {}
    void receive(ECXEvent& e) override {
        int id = std::any_cast<int>(e.args[0]);
        ids.push_back(id);
        if (id == 1) {
            ECXEvent next = makeEvent(ECXEventType::EntityCreate, 2);
            broker.publish(next);
        }
    }
};

constexpr int kThreads = 4;
constexpr int kPerThread = 2000;

// Every id in [0, kThreads*kPerThread) must appear exactly once, and each producer thread's own
// ids must arrive in the order that thread published them.
void requireExactlyOnceInPerProducerOrder(const std::vector<int>& ids) {
    REQUIRE(ids.size() == static_cast<size_t>(kThreads * kPerThread));
    std::vector<int> seen(kThreads * kPerThread, 0);
    std::vector<int> last(kThreads, -1);
    bool ordered = true;
    for (int id : ids) {
        REQUIRE(id >= 0);
        REQUIRE(id < kThreads * kPerThread);
        seen[id]++;
        int producer = id / kPerThread;
        if (id <= last[producer]) ordered = false;
        last[producer] = id;
    }
    bool onceEach = true;
    for (int n : seen) onceEach = onceEach && (n == 1);
    REQUIRE(onceEach);
    REQUIRE(ordered);
}

} // namespace

TEST_CASE("an event with no subscriber is dropped instead of blocking flush", "[broker][broker-drop]") {
    auto broker = std::make_shared<ECXEventBroker>();
    auto listener = std::make_shared<RecordingEventListener>();
    broker->Subscribe(*listener, ECXEventType::EntityCreate);

    // The unsubscribed event is published first so it sits at the front of the queue.
    ECXEvent orphan = makeEvent(ECXEventType::EntityKill, 1);
    ECXEvent wanted = makeEvent(ECXEventType::EntityCreate, 2);
    broker->publish(orphan);
    broker->publish(wanted);

    REQUIRE(completesWithin(std::chrono::milliseconds(2000), [broker, listener] { broker->flush(); }));
    REQUIRE(listener->ids == std::vector<int>{ 2 });
}

TEST_CASE("events are delivered in publish order to every subscriber of their type", "[broker]") {
    ECXEventBroker broker;
    RecordingEventListener first;
    RecordingEventListener second;
    broker.Subscribe(first, ECXEventType::EntityCreate);
    broker.Subscribe(second, ECXEventType::EntityCreate);

    for (int i = 0; i < 5; i++) {
        ECXEvent e = makeEvent(ECXEventType::EntityCreate, i);
        broker.publish(e);
    }
    broker.flush();

    REQUIRE(first.ids == std::vector<int>{ 0, 1, 2, 3, 4 });
    REQUIRE(second.ids == std::vector<int>{ 0, 1, 2, 3, 4 });
}

TEST_CASE("an event published from inside a listener is delivered on the next flush", "[broker]") {
    ECXEventBroker broker;
    RepublishingListener listener(broker);
    broker.Subscribe(listener, ECXEventType::EntityCreate);

    ECXEvent first = makeEvent(ECXEventType::EntityCreate, 1);
    broker.publish(first);

    broker.flush();
    REQUIRE(listener.ids == std::vector<int>{ 1 });

    broker.flush();
    REQUIRE(listener.ids == std::vector<int>{ 1, 2 });
}

TEST_CASE("events published from several threads while flush runs are delivered exactly once", "[broker][broker-threads]") {
    ECXEventBroker broker;
    RecordingEventListener listener;
    broker.Subscribe(listener, ECXEventType::EntityCreate);

    std::atomic<int> finished{ 0 };
    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; t++) {
        producers.emplace_back([&broker, &finished, t] {
            for (int i = 0; i < kPerThread; i++) {
                ECXEvent e = makeEvent(ECXEventType::EntityCreate, t * kPerThread + i);
                broker.publish(e);
            }
            finished++;
        });
    }
    while (finished < kThreads) broker.flush();
    for (auto& p : producers) p.join();
    broker.flush();
    broker.flush();

    requireExactlyOnceInPerProducerOrder(listener.ids);
}

TEST_CASE("a command with no subscriber is dropped", "[broker]") {
    ECXCommandBroker broker;
    RecordingCommandListener listener;
    broker.Subscribe(listener, ECXCommandType::GamePause);

    ECXCommand orphan = makeCommand(ECXCommandType::GameResume, 1);
    ECXCommand wanted = makeCommand(ECXCommandType::GamePause, 2);
    broker.publish(orphan);
    broker.publish(wanted);
    broker.flush();

    REQUIRE(listener.ids == std::vector<int>{ 2 });
}

TEST_CASE("commands published from several threads while flush runs are delivered exactly once", "[broker][broker-threads]") {
    ECXCommandBroker broker;
    RecordingCommandListener listener;
    broker.Subscribe(listener, ECXCommandType::GamePause);

    std::atomic<int> finished{ 0 };
    std::vector<std::thread> producers;
    for (int t = 0; t < kThreads; t++) {
        producers.emplace_back([&broker, &finished, t] {
            for (int i = 0; i < kPerThread; i++) {
                ECXCommand c = makeCommand(ECXCommandType::GamePause, t * kPerThread + i);
                broker.publish(c);
            }
            finished++;
        });
    }
    while (finished < kThreads) broker.flush();
    for (auto& p : producers) p.join();
    broker.flush();
    broker.flush();

    requireExactlyOnceInPerProducerOrder(listener.ids);
}
