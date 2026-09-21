// ErrorBus.cpp
#include "ErrorBus.hpp"
#include <mutex>

namespace ANI::ErrorBus {
    namespace {
        std::mutex         g_mutex;
        std::vector<Entry> g_pending;
        uint64_t           g_nextId = 1;
        WakeFn             g_wake = nullptr;
        void* g_wakeUser = nullptr;
    }

    void Push(const char* source, const std::string& message,
        const char* file, int line) {
        bool wasEmpty = false;
        WakeFn wake = nullptr;
        void* wakeUser = nullptr;

        {
            std::lock_guard<std::mutex> lk(g_mutex);
            wasEmpty = g_pending.empty();
            Entry e;
            e.id = g_nextId++;
            e.source = source ? source : "Error";
            e.message = message;
            e.file = file ? file : "";
            e.line = line;
            g_pending.push_back(std::move(e));
            wake = g_wake;
            wakeUser = g_wakeUser;
        }

        if (wasEmpty && wake) {
            wake(wakeUser);   // called outside the lock
        }
    }

    std::vector<Entry> Drain() {
        std::lock_guard<std::mutex> lk(g_mutex);
        std::vector<Entry> out;
        out.swap(g_pending);
        return out;
    }

    size_t PendingCount() {
        std::lock_guard<std::mutex> lk(g_mutex);
        return g_pending.size();
    }

    void SetWakeCallback(WakeFn fn, void* user) {
        std::lock_guard<std::mutex> lk(g_mutex);
        g_wake = fn;
        g_wakeUser = user;
    }
}