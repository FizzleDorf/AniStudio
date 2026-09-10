#include "Discovery.hpp"
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <sstream>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
using socklen_t = int;
#define NET_CLOSE closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#define NET_CLOSE ::close
#endif

namespace Net {

    namespace {

        void InitSocketsOnce() {
#ifdef _WIN32
            static bool done = false;
            if (!done) {
                WSADATA wsa;
                WSAStartup(MAKEWORD(2, 2), &wsa);
                done = true;
            }
#endif
        }

        std::string BuildProbeResponse(const ServerInfo& info) {
            nlohmann::json j;
            j["name"] = info.host;
            j["project"] = info.projectName;
            j["projectPath"] = info.projectPath;
            j["hostUser"] = info.hostUser;
            j["tcpPort"] = info.tcpPort;
            j["clients"] = info.clientCount;
            j["maxClients"] = info.maxClients;
            return j.dump();
        }

        bool ParseProbeResponse(const std::string& s, const std::string& fromAddr, ServerInfo& out) {
            try {
                auto j = nlohmann::json::parse(s);
                out.host = j.value("name", "unknown");
                out.address = fromAddr;
                out.projectName = j.value("project", "");
                out.projectPath = j.value("projectPath", "");
                out.hostUser = j.value("hostUser", "");
                out.tcpPort = j.value("tcpPort", (uint16_t)9000);
                out.clientCount = j.value("clients", 0u);
                out.maxClients = j.value("maxClients", 8u);
                return !out.projectName.empty();
            }
            catch (...) {
                return false;
            }
        }

    } // anon

    struct DiscoveryHandle {
        std::thread       thread;
        std::atomic<bool> running{ false };
        int               sock = -1;
        ServerInfo        info;
    };

    DiscoveryHandle* StartDiscoveryResponder(const ServerInfo& info) {
        InitSocketsOnce();

        auto* h = new DiscoveryHandle();
        h->info = info;
        h->running = true;

        h->sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
        if (h->sock < 0) {
            delete h;
            return nullptr;
        }

        int opt = 1;
#ifdef _WIN32
        setsockopt(h->sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        BOOL bcast = TRUE;
        setsockopt(h->sock, SOL_SOCKET, SO_BROADCAST, (const char*)&bcast, sizeof(bcast));
#else
        setsockopt(h->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        int bcast = 1;
        setsockopt(h->sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(9001);

        if (bind(h->sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::cerr << "[Discovery] bind(9001) failed; discovery disabled\n";
            NET_CLOSE(h->sock);
            delete h;
            return nullptr;
        }

        h->thread = std::thread([h]() {
            char buf[256];
            sockaddr_in from{};
            socklen_t fromLen = sizeof(from);
            while (h->running) {
                int n = (int)recvfrom(h->sock, buf, sizeof(buf) - 1, 0,
                    (sockaddr*)&from, &fromLen);
                if (n <= 0) continue;
                buf[n] = 0;
                if (std::strcmp(buf, "ANISTUDIO_DISCOVER") != 0) continue;

                std::string resp = BuildProbeResponse(h->info);
                sendto(h->sock, resp.c_str(), (int)resp.size(), 0,
                    (sockaddr*)&from, fromLen);
            }
            });

        std::cout << "[Discovery] Responder listening on UDP 9001\n";
        return h;
    }

    void StopDiscoveryResponder(DiscoveryHandle* h) {
        if (!h) return;
        h->running = false;
        if (h->sock >= 0) NET_CLOSE(h->sock);
        if (h->thread.joinable()) h->thread.join();
        delete h;
    }

    std::vector<ServerInfo> DiscoverServers(uint16_t discoveryPort, int timeoutMs) {
        InitSocketsOnce();

        std::vector<ServerInfo> found;

        int sock = (int)socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return found;

#ifdef _WIN32
        BOOL bcast = TRUE;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&bcast, sizeof(bcast));
        DWORD tv = timeoutMs;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
#else
        int bcast = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));
        timeval tv{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

        sockaddr_in target{};
        target.sin_family = AF_INET;
        target.sin_addr.s_addr = htonl(INADDR_BROADCAST);
        target.sin_port = htons(discoveryPort);

        const char* probe = "ANISTUDIO_DISCOVER";
        sendto(sock, probe, (int)std::strlen(probe), 0,
            (sockaddr*)&target, sizeof(target));

        auto deadline = std::chrono::steady_clock::now()
            + std::chrono::milliseconds(timeoutMs);

        while (std::chrono::steady_clock::now() < deadline) {
            char buf[1024];
            sockaddr_in from{};
            socklen_t fromLen = sizeof(from);
            int n = (int)recvfrom(sock, buf, sizeof(buf) - 1, 0,
                (sockaddr*)&from, &fromLen);
            if (n <= 0) break;
            buf[n] = 0;

            char addrStr[INET_ADDRSTRLEN] = { 0 };
            inet_ntop(AF_INET, &from.sin_addr, addrStr, sizeof(addrStr));

            ServerInfo info;
            if (ParseProbeResponse(std::string(buf, n), addrStr, info)) {
                bool dup = false;
                for (auto& s : found) {
                    if (s.address == info.address && s.tcpPort == info.tcpPort) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) found.push_back(std::move(info));
            }
        }

        NET_CLOSE(sock);
        return found;
    }

} // namespace Net