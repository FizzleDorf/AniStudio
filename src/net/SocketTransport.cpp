#include "SocketTransport.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socklen_t = int;
#define NET_CLOSE closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#define NET_CLOSE ::close
#endif

namespace Net {

    SocketTransport::~SocketTransport() { Close(); }

    bool SocketTransport::Listen(uint16_t port) {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        listenFd = (int)socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd < 0) return false;

        int opt = 1;
#ifdef _WIN32
        setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
        setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(listenFd, (sockaddr*)&addr, sizeof(addr)) < 0) return false;
        if (listen(listenFd, 16) < 0) return false;
        return true;
    }

    int SocketTransport::Accept() {
        return (int)accept(listenFd, nullptr, nullptr);
    }

    void SocketTransport::CloseListener() {
        if (listenFd >= 0) { NET_CLOSE(listenFd); listenFd = -1; }
    }

    bool SocketTransport::Connect(const std::string& host, uint16_t port) {
#ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
        fd = (int)socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return false;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) return false;

        return connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0;
    }

    void SocketTransport::Close() {
        if (fd >= 0) { NET_CLOSE(fd); fd = -1; }
        if (listenFd >= 0) { NET_CLOSE(listenFd); listenFd = -1; }
    }

    bool SocketTransport::ReadExact(void* buf, size_t n) {
        char* p = (char*)buf;
        while (n) {
#ifdef _WIN32
            int r = recv(fd, p, (int)n, 0);
#else
            ssize_t r = ::recv(fd, p, n, 0);
#endif
            if (r <= 0) return false;
            p += r; n -= (size_t)r;
        }
        return true;
    }

    bool SocketTransport::WriteExact(const void* buf, size_t n) {
        const char* p = (const char*)buf;
        while (n) {
#ifdef _WIN32
            int r = send(fd, p, (int)n, 0);
#else
            ssize_t r = ::send(fd, p, n, MSG_NOSIGNAL);
#endif
            if (r <= 0) return false;
            p += r; n -= (size_t)r;
        }
        return true;
    }

} // namespace Net