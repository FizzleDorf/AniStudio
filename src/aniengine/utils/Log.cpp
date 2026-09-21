#include "Log.hpp"
#include "ErrorBus.hpp"

#include <cstdio>
#include <cstring>
#include <atomic>
#include <string>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <system_error>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define ANI_LOG_FILENO    _fileno
#define ANI_LOG_DUP(fd)   _dup(fd)
#define ANI_LOG_DUP2(a,b) _dup2((a),(b))
#define ANI_LOG_CLOSE(fd) _close(fd)
#define ANI_LOG_READ(fd,buf,n) _read((fd),(buf),(unsigned int)(n))
#define ANI_LOG_WRITE(fd,buf,n) _write((fd),(buf),(unsigned int)(n))
#define ANI_LOG_PIPE(fds) _pipe((fds), 65536, _O_BINARY)
#else
#include <unistd.h>
#define ANI_LOG_FILENO    ::fileno
#define ANI_LOG_DUP(fd)   ::dup(fd)
#define ANI_LOG_DUP2(a,b) ::dup2((a),(b))
#define ANI_LOG_CLOSE(fd) ::close(fd)
#define ANI_LOG_READ(fd,buf,n) ::read((fd),(buf),(n))
#define ANI_LOG_WRITE(fd,buf,n) ::write((fd),(buf),(n))
#define ANI_LOG_PIPE(fds) ::pipe((fds))
#endif

namespace ANI::Log {

    namespace {

        std::atomic<int> g_level{ static_cast<int>(Level::Trace) };

        SinkFn g_sink = nullptr;
        void* g_sinkUser = nullptr;

        // Session file + redirect plumbing.
        std::FILE* g_file = nullptr;
        int         g_savedStderrFd = -1;
        int         g_savedStdoutFd = -1;
        int         g_pipeWriteFd = -1;
        int         g_pipeReadFd = -1;
        std::string g_path;

        std::thread             g_pumpThread;
        std::atomic<bool>       g_pumpRunning{ false };
        std::mutex              g_pumpMutex;
        std::condition_variable g_pumpCv;

        thread_local char g_pathBuf[512];

        struct Stamp { int year, mon, day, hour, min, sec, ms; };

        Stamp Now() {
            using namespace std::chrono;
            auto now = system_clock::now();
            auto t = system_clock::to_time_t(now);
            auto ms = static_cast<int>(
                (duration_cast<milliseconds>(now.time_since_epoch()) % 1000).count());

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

        // Drains the pipe and mirrors to the saved stderr fd and the file.
        // Runs on a dedicated thread for the whole lifetime of the session.
        void PumpLoop() {
            std::vector<char> buf(8192);

            while (g_pumpRunning.load(std::memory_order_relaxed)) {
                int n = ANI_LOG_READ(g_pipeReadFd, buf.data(),
                    static_cast<int>(buf.size()));
                if (n <= 0) {
                    std::unique_lock<std::mutex> lk(g_pumpMutex);
                    g_pumpCv.wait_for(lk, std::chrono::milliseconds(20));
                    continue;
                }

                if (g_savedStderrFd >= 0) {
                    ANI_LOG_WRITE(g_savedStderrFd, buf.data(), n);
                }
                if (g_file) {
                    std::fwrite(buf.data(), 1, static_cast<size_t>(n), g_file);
                    std::fflush(g_file);
                }
            }

            // Final flush of whatever is still in the pipe.
            for (;;) {
                int n = ANI_LOG_READ(g_pipeReadFd, buf.data(),
                    static_cast<int>(buf.size()));
                if (n <= 0) break;

                if (g_savedStderrFd >= 0) {
                    ANI_LOG_WRITE(g_savedStderrFd, buf.data(), n);
                }
                if (g_file) {
                    std::fwrite(buf.data(), 1, static_cast<size_t>(n), g_file);
                    std::fflush(g_file);
                }
            }
        }

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

            int savedErr = ANI_LOG_DUP(stderrFd);
            if (savedErr < 0) { std::fclose(f); return false; }

            int savedOut = -1;
            if (stdoutFd >= 0) {
                savedOut = ANI_LOG_DUP(stdoutFd);
                if (savedOut < 0) {
                    ANI_LOG_CLOSE(savedErr);
                    std::fclose(f);
                    return false;
                }
            }

            int pipeFds[2] = { -1, -1 };
            if (ANI_LOG_PIPE(pipeFds) != 0) {
                ANI_LOG_CLOSE(savedErr);
                if (savedOut >= 0) ANI_LOG_CLOSE(savedOut);
                std::fclose(f);
                return false;
            }

            g_file = f;
            g_savedStderrFd = savedErr;
            g_savedStdoutFd = savedOut;
            g_pipeReadFd = pipeFds[0];
            g_pipeWriteFd = pipeFds[1];
            g_path = path;

            if (ANI_LOG_DUP2(g_pipeWriteFd, stderrFd) != 0) {
                ANI_LOG_CLOSE(g_pipeReadFd);
                ANI_LOG_CLOSE(g_pipeWriteFd);
                ANI_LOG_CLOSE(savedErr);
                if (savedOut >= 0) ANI_LOG_CLOSE(savedOut);
                std::fclose(f);
                g_file = nullptr; g_pipeReadFd = -1; g_pipeWriteFd = -1;
                g_savedStderrFd = -1; g_savedStdoutFd = -1; g_path.clear();
                return false;
            }

            if (savedOut >= 0 && ANI_LOG_DUP2(g_pipeWriteFd, stdoutFd) != 0) {
                ANI_LOG_DUP2(savedErr, stderrFd);
                ANI_LOG_CLOSE(g_pipeReadFd);
                ANI_LOG_CLOSE(g_pipeWriteFd);
                ANI_LOG_CLOSE(savedErr);
                ANI_LOG_CLOSE(savedOut);
                std::fclose(f);
                g_file = nullptr; g_pipeReadFd = -1; g_pipeWriteFd = -1;
                g_savedStderrFd = -1; g_savedStdoutFd = -1; g_path.clear();
                return false;
            }

            g_pumpRunning.store(true, std::memory_order_relaxed);
            g_pumpThread = std::thread(PumpLoop);

            std::setvbuf(stdout, nullptr, _IONBF, 0);
            std::setvbuf(stderr, nullptr, _IONBF, 0);

            return true;
        }

        void RedirectEnd() {
            if (!g_file) return;

            std::fflush(stdout);
            std::fflush(stderr);

            if (g_savedStderrFd >= 0) {
                ANI_LOG_DUP2(g_savedStderrFd, ANI_LOG_FILENO(stderr));
            }
            if (g_savedStdoutFd >= 0) {
                ANI_LOG_DUP2(g_savedStdoutFd, ANI_LOG_FILENO(stdout));
            }

            g_pumpRunning.store(false, std::memory_order_relaxed);
            {
                std::lock_guard<std::mutex> lk(g_pumpMutex);
                g_pumpCv.notify_all();
            }

            // Closing the write end unblocks the pump's read().
            if (g_pipeWriteFd >= 0) {
                ANI_LOG_CLOSE(g_pipeWriteFd);
                g_pipeWriteFd = -1;
            }

            if (g_pumpThread.joinable()) {
                g_pumpThread.join();
            }

            if (g_pipeReadFd >= 0) {
                ANI_LOG_CLOSE(g_pipeReadFd);
                g_pipeReadFd = -1;
            }
            if (g_savedStderrFd >= 0) {
                ANI_LOG_CLOSE(g_savedStderrFd);
                g_savedStderrFd = -1;
            }
            if (g_savedStdoutFd >= 0) {
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

    void Emit(Level lvl, const char* file, int line, const char* fmt, ...) {
        if (lvl == Level::Off) return;

        char msg[2048];
        va_list args;
        va_start(args, fmt);
        std::vsnprintf(msg, sizeof(msg), fmt, args);
        va_end(args);

        // Errors always reach the bus regardless of the console verbosity
        // setting. This is what drives the gui popups.
        if (lvl == Level::Error) {
            ANI::ErrorBus::Push("AniStudio", msg, file, line);
        }

        if (static_cast<int>(lvl) < g_level.load(std::memory_order_relaxed)) return;

        char timeBuf[32];
        FormatTimePrefix(timeBuf, sizeof(timeBuf));

        char prefix[320];
        std::snprintf(prefix, sizeof(prefix), "[%s][%-5s][%s:%d] ",
            timeBuf, LevelName(lvl), ShortFile(file), line);

        std::fprintf(stderr, "%s%s\n", prefix, msg);

        if (g_sink) {
            g_sink(lvl, file, line, msg, g_sinkUser);
        }
    }

} // namespace ANI::Log