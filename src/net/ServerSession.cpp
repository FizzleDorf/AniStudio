#include "ServerSession.hpp"
#include <iostream>

namespace Net {

    ServerSession::ServerSession(int fd, SessionID sid, ProjectID pid,
        SubscriptionRouter& r,
        std::function<void(SessionID, MsgType, const void*, size_t)> cb)
        : sessionID(sid), projectID(pid), router(r), onCommand(std::move(cb))
    {
        // Reuse the accepted fd. SocketTransport doesn't own it via Connect, so
        // we assign it directly. We need a way to set it ? expose a setter or
        // make ServerSession hold the fd. For clarity we add a small helper.
        transport.Close();
        // NOTE: we need a way to give SocketTransport an existing fd.
        // Simplest: add `void AdoptFd(int)` to SocketTransport. See below.
        transport.AdoptFd(fd);
    }

    ServerSession::~ServerSession() {
        Stop();
    }

    void ServerSession::Start() {
        running = true;
        rxThread = std::thread(&ServerSession::ReadLoop, this);
    }

    void ServerSession::Stop() {
        running = false;
        transport.Close();
        if (rxThread.joinable()) rxThread.join();
    }

    bool ServerSession::SendRaw(const void* data, size_t size) {
        std::lock_guard<std::mutex> lk(sendMtx);
        return transport.WriteExact(data, size);
    }

    void ServerSession::ReadLoop() {
        while (running) {
            MsgHeader h;
            if (!transport.ReadExact(&h, sizeof(h))) break;

            std::vector<char> payload(h.size);
            if (h.size > 0 && !transport.ReadExact(payload.data(), h.size)) break;

            if (onCommand) onCommand(sessionID, h.type, payload.data(), payload.size());
        }
        running = false;
    }

} // namespace Net