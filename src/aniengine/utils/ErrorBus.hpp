// ErrorBus.hpp
#pragma once
#include "AniEngine.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace ANI::ErrorBus {

    struct Entry {
        uint64_t    id = 0;
        std::string source;   // "sd.cpp" / "AniStudio" / etc.
        std::string message;
        std::string file;     // may be empty
        int         line = 0;
    };

    // Safe to call from any thread
    ANI_ENGINE_API void Push(const char* source,
        const std::string& message,
        const char* file = nullptr,
        int line = 0);

    // Copies and clears the pending list. Call from the UI thread once per frame.
    ANI_ENGINE_API std::vector<Entry> Drain();

    // Count without draining, for a badge or "N new errors" indicator.
    ANI_ENGINE_API size_t PendingCount();

    // Called synchronously on the pushing thread when the first
    // entry lands in an empty queue. Useful if you later want to raise a
    // native notification even when the window isn't focused.
    using WakeFn = void(*)(void* user);
    ANI_ENGINE_API void SetWakeCallback(WakeFn fn, void* user = nullptr);

} // namespace ANI::ErrorBus