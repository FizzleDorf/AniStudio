#pragma once
#include "Protocol.hpp"
#include "SocketTransport.hpp"
#include "ClientMirror.hpp"
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Net {

    class NetClient {
    public:
        NetClient() = default;
        ~NetClient() { Disconnect(); }

        bool Connect(const std::string& host, uint16_t port, const std::string& name);
        void Disconnect();

        void SendMoveCursor(float x, float y);
        void SendSetProperty(uint64_t entity, uint32_t comp,
            uint32_t version,
            const std::string& key,
            const std::string& value);

        ClientMirror& Mirror() { return mirror; }
        const ClientMirror& Mirror() const { return mirror; }

        bool IsConnected() const { return running.load(); }

    private:
        void ReadLoop();
        void Handle(MsgType t, const void* d, size_t n);

        SocketTransport    transport;
        std::thread        rx;
        std::atomic<bool>  running{ false };
        std::mutex         txMtx;
        ClientMirror       mirror;
    };

} // namespace Net