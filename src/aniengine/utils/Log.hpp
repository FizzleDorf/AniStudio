#pragma once

#include "AniEngine.hpp"
#include <cstdarg>

namespace ANI::Log {

    enum class Level : int {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Off = 5
    };

    // Minimum level written to stderr/file. Defaults to Trace so nothing
    // is dropped during startup; SettingsSystem lowers it after load.
    ANI_ENGINE_API void  SetLevel(Level l) noexcept;
    ANI_ENGINE_API Level GetLevel()        noexcept;

    // Optional mirror to an in-app console. Pass nullptr to clear.
    using SinkFn = void(*)(Level lvl,
        const char* file,
        int line,
        const char* msg,
        void* user);
    ANI_ENGINE_API void SetSink(SinkFn fn, void* user = nullptr);

    // Session file. Open once, early. Redirects stderr+stdout into
    // <logDir>/session_<timestamp>.log so third-party code (sd.cpp,
    // ggml, ffmpeg, ImGui) is captured too. False if already open
    // or the file could not be created.
    ANI_ENGINE_API bool SessionOpen(const char* logDir);
    ANI_ENGINE_API bool SessionOpenFile(const char* fullPath);
    ANI_ENGINE_API void SessionClose();
    ANI_ENGINE_API bool SessionIsOpen();
    ANI_ENGINE_API const char* SessionPath();

    // Builds "<dir>/session_YYYY-MM-DD_HH-MM-SS.log" without opening.
    ANI_ENGINE_API const char* MakeSessionFilename(const char* dir);

    // Single funnel for every macro. Writes to stderr (session file
    // when open), forwards to the sink, and pushes Error-level lines
    // to ErrorBus unconditionally.
    ANI_ENGINE_API void Emit(Level lvl,
        const char* file,
        int line,
        const char* fmt, ...);

} // namespace ANI::Log

#define ANI_LOG_TRACE(...) ::ANI::Log::Emit(::ANI::Log::Level::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_DEBUG(...) ::ANI::Log::Emit(::ANI::Log::Level::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_INFO(...)  ::ANI::Log::Emit(::ANI::Log::Level::Info,  __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_WARN(...)  ::ANI::Log::Emit(::ANI::Log::Level::Warn,  __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_ERROR(...) ::ANI::Log::Emit(::ANI::Log::Level::Error, __FILE__, __LINE__, __VA_ARGS__)