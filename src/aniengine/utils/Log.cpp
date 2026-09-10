#include "Log.hpp"

#include <cstdio>
#include <cstring>
#include <atomic>
#include <string>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
#include <io.h>
#define ANI_LOG_FILENO    _fileno
#define ANI_LOG_DUP(fd)   _dup(fd)
#define ANI_LOG_DUP2(a,b) _dup2((a),(b))
#define ANI_LOG_CLOSE(fd) _close(fd)
#else
#include <unistd.h>
#define ANI_LOG_FILENO    ::fileno
#define ANI_LOG_DUP(fd)   ::dup(fd)
#define ANI_LOG_DUP2(a,b) ::dup2((a),(b))
#define ANI_LOG_CLOSE(fd) ::close(fd)
#endif

namespace ANI::Log {

    namespace {

        // Single copy of all logger state. Lives in AniEngineCore.dll only,
        // because this .cpp is compiled into that target. Every other module
        // calls the exported functions, so there is exactly one g_level, one
        // g_file, one set of redirected fds across the whole process.
        std::atomic<int> g_level{ static_cast<int>(Level::Trace) };

        SinkFn g_sink = nullptr;
        void* g_sinkUser = nullptr;

        std::FILE* g_file = nullptr;
        int         g_savedStderrFd = -1;
        int         g_savedStdoutFd = -1;
        std::string g_path;

        // Thread-local buffer for MakeSessionFilename and SessionPath so the
        // caller gets a valid pointer without worrying about a shared
        // std::string being mutated under them. 512 bytes is plenty for any
        // reasonable Windows path plus the filename.
        thread_local char g_pathBuf[512];

        struct Stamp { int year, mon, day, hour, min, sec, ms; };

        Stamp Now() {
            using namespace std::chrono;
            auto now = system_clock::now();
            auto t = system_clock::to_time_t(now);
            auto ms = static_cast<int>(
                (duration_cast<milliseconds>(now.time_since_epoch()) % 1000).count()
                );

            std::tm tm{};
#ifdef _WIN32
            localtime_s(&tm, &t);
#else
            localtime_r(&t, &tm);
#endif

            return { tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                     tm.tm_hour, tm.tm_min, tm.tm_sec, ms };
        }

        void FormatTimePrefix(char* out, size_t n) {
            auto s = Now();
            std::snprintf(out, n, "%02d:%02d:%02d.%03d",
                s.hour, s.min, s.sec, s.ms);
        }

        const char* LevelName(Level l) {
            switch (l) {
            case Level::Trace: return "TRACE";
            case Level::Debug: return "DEBUG";
            case Level::Info:  return "INFO";
            case Level::Warn:  return "WARN";
            case Level::Error: return "ERROR";
            default:           return "OFF";
            }
        }

        // Strips everything up to the last slash or backslash so log lines
        // read "AniEngine.cpp:29" instead of the full absolute path.
        const char* ShortFile(const char* f) {
            if (!f) return "?";
            const char* last = f;
            for (const char* p = f; *p; ++p) {
#ifdef _WIN32
                if (*p == '\\' || *p == '/') last = p + 1;
#else
                if (*p == '/') last = p + 1;
#endif
            }
            return last;
        }

        // Opens the file and points stderr and stdout at it. On any failure
        // every fd opened so far is closed and no globals are touched, so
        // the caller can safely retry or fall back.
        bool RedirectBegin(const std::string& path) {
            if (g_file) return false;

            std::error_code ec;
            std::filesystem::path pp(path);
            if (pp.has_parent_path()) {
                std::filesystem::create_directories(pp.parent_path(), ec);
            }

            std::FILE* f = nullptr;
#ifdef _WIN32
            errno_t err = fopen_s(&f, path.c_str(), "a");
            if (err != 0 || !f) return false;
#else
            f = std::fopen(path.c_str(), "a");
            if (!f) return false;
#endif

            int stderrFd = ANI_LOG_FILENO(stderr);
            int stdoutFd = ANI_LOG_FILENO(stdout);

            int dupErr = ANI_LOG_DUP(stderrFd);
            if (dupErr < 0) { std::fclose(f); return false; }

            int dupOut = -1;
            if (stdoutFd >= 0) {
                dupOut = ANI_LOG_DUP(stdoutFd);
                if (dupOut < 0) {
                    ANI_LOG_CLOSE(dupErr);
                    std::fclose(f);
                    return false;
                }
            }

            int fileFd = ANI_LOG_FILENO(f);

            if (ANI_LOG_DUP2(fileFd, stderrFd) != 0) {
                ANI_LOG_CLOSE(dupErr);
                if (dupOut >= 0) ANI_LOG_CLOSE(dupOut);
                std::fclose(f);
                return false;
            }

            if (dupOut >= 0 && ANI_LOG_DUP2(fileFd, stdoutFd) != 0) {
                ANI_LOG_DUP2(dupErr, stderrFd);
                ANI_LOG_CLOSE(dupErr);
                ANI_LOG_CLOSE(dupOut);
                std::fclose(f);
                return false;
            }

            g_file = f;
            g_savedStderrFd = dupErr;
            g_savedStdoutFd = dupOut;
            g_path = path;
            return true;
        }

        void RedirectEnd() {
            if (!g_file) return;

            // Push anything sitting in the FILE buffers into the file before
            // we restore the original fds, otherwise later writes to stderr
            // would go to the console while older buffered lines were still
            // pending.
            std::fflush(stdout);
            std::fflush(stderr);
            std::fflush(g_file);

            if (g_savedStderrFd >= 0) {
                ANI_LOG_DUP2(g_savedStderrFd, ANI_LOG_FILENO(stderr));
                ANI_LOG_CLOSE(g_savedStderrFd);
                g_savedStderrFd = -1;
            }
            if (g_savedStdoutFd >= 0) {
                ANI_LOG_DUP2(g_savedStdoutFd, ANI_LOG_FILENO(stdout));
                ANI_LOG_CLOSE(g_savedStdoutFd);
                g_savedStdoutFd = -1;
            }

            std::fclose(g_file);
            g_file = nullptr;
            g_path.clear();
        }

    } // anonymous namespace

    void  SetLevel(Level l) noexcept { g_level.store(static_cast<int>(l), std::memory_order_relaxed); }
    Level GetLevel()        noexcept { return static_cast<Level>(g_level.load(std::memory_order_relaxed)); }

    void SetSink(SinkFn fn, void* user) {
        g_sink = fn;
        g_sinkUser = user;
    }

    const char* MakeSessionFilename(const char* dir) {
        const char* d = (dir && *dir) ? dir : "./data/session_logs";
        auto s = Now();
        std::snprintf(g_pathBuf, sizeof(g_pathBuf),
            "%s/session_%04d-%02d-%02d_%02d-%02d-%02d.log",
            d, s.year, s.mon, s.day, s.hour, s.min, s.sec);
        return g_pathBuf;
    }

    bool SessionOpen(const char* logDir) {
        const char* d = (logDir && *logDir) ? logDir : "./data/session_logs";
        return RedirectBegin(MakeSessionFilename(d));
    }

    bool SessionOpenFile(const char* fullPath) {
        if (!fullPath || !*fullPath) return false;
        return RedirectBegin(fullPath);
    }

    void SessionClose() {
        RedirectEnd();
    }

    bool SessionIsOpen() {
        return g_file != nullptr;
    }

    const char* SessionPath() {
        std::snprintf(g_pathBuf, sizeof(g_pathBuf), "%s", g_path.c_str());
        return g_pathBuf;
    }

    void Emit(Level lvl,
        const char* file,
        int line,
        const char* fmt, ...)
    {
        if (static_cast<int>(lvl) < g_level.load(std::memory_order_relaxed)) return;
        if (lvl == Level::Off) return;

        char msg[2048];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        char timeBuf[32];
        FormatTimePrefix(timeBuf, sizeof(timeBuf));

        // "%-5s" left-justifies the level tag into five columns so the
        // bracket after it lines up across INFO/TRACE/ERROR.
        char prefix[320];
        std::snprintf(prefix, sizeof(prefix), "[%s][%-5s][%s:%d] ",
            timeBuf, LevelName(lvl), ShortFile(file), line);

        std::fprintf(stderr, "%s%s\n", prefix, msg);

        if (g_sink) {
            g_sink(lvl, file, line, msg, g_sinkUser);
        }
    }

} // namespace ANI::Log