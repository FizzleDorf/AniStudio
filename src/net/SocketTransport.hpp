#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

namespace Net {

    // Minimal blocking TCP. Swap for uWebSockets / emscripten_websocket_* later.
    class SocketTransport {
    public:
        SocketTransport() = default;
        ~SocketTransport();

        // Server side
        bool Listen(uint16_t port);
        int  Accept();                       // returns fd, -1 on error
        void CloseListener();

        // Client side
        bool Connect(const std::string& host, uint16_t port);

        // Both
        void Close();

        // Blocking read/write. Return false on disconnect.
        bool ReadExact(void* buf, size_t n);
        bool WriteExact(const void* buf, size_t n);

        void AdoptFd(int existingFd) { fd = existingFd; }

        int Fd() const { return fd; }

    private:
        int  fd = -1;
        int  listenFd = -1;
    };

} // namespace Net