#pragma once
#include "SocketTransport.hpp"
#include "SubscriptionRouter.hpp"
#include "Protocol.hpp"
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>

namespace Net {

    class ServerSession {
    public:
        ServerSession(int fd, SessionID sid, ProjectID pid,
            SubscriptionRouter& router,
            std::function<void(SessionID, MsgType, const void*, size_t)> onCommand);

        ~ServerSession();

        void Start();           // spawns read thread
        void Stop();

        bool SendRaw(const void* data, size_t size);

        SessionID ID() const { return sessionID; }
        ProjectID Project() const { return projectID; }

    private:
        void ReadLoop();

        SocketTransport                    transport;
        SessionID                          sessionID;
        ProjectID                          projectID;
        SubscriptionRouter& router;
        std::function<void(SessionID, MsgType, const void*, size_t)> onCommand;

        std::thread                        rxThread;
        std::atomic<bool>                  running{ false };
        std::mutex                         sendMtx;
    };

} // namespace Net