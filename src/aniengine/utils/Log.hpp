#pragma once

#include "EngineExport.hpp"
#include <cstdarg>

namespace ANI::Log {

    enum class Level : int {
        Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4, Off = 5
    };

    ANI_ENGINE_API void  SetLevel(Level l) noexcept;
    ANI_ENGINE_API Level GetLevel()        noexcept;

    using SinkFn = void(*)(Level lvl, const char* file, int line,
                           const char* msg, void* user);
    ANI_ENGINE_API void SetSink(SinkFn fn, void* user = nullptr);

    ANI_ENGINE_API bool SessionOpen(const char* logDir);
    ANI_ENGINE_API bool SessionOpenFile(const char* fullPath);
    ANI_ENGINE_API void SessionClose();
    ANI_ENGINE_API bool SessionIsOpen();
    ANI_ENGINE_API const char* SessionPath();
    ANI_ENGINE_API const char* MakeSessionFilename(const char* dir);

    ANI_ENGINE_API void Emit(Level lvl, const char* file, int line,
                             const char* fmt, ...);

}

#define ANI_LOG_TRACE(...) ::ANI::Log::Emit(::ANI::Log::Level::Trace, __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_DEBUG(...) ::ANI::Log::Emit(::ANI::Log::Level::Debug, __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_INFO(...)  ::ANI::Log::Emit(::ANI::Log::Level::Info,  __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_WARN(...)  ::ANI::Log::Emit(::ANI::Log::Level::Warn,  __FILE__, __LINE__, __VA_ARGS__)
#define ANI_LOG_ERROR(...) ::ANI::Log::Emit(::ANI::Log::Level::Error, __FILE__, __LINE__, __VA_ARGS__)