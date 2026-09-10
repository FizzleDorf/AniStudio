#include "NetClient.hpp"
#include <cstring>
#include <iostream>

namespace Net {

    bool NetClient::Connect(const std::string& host, uint16_t port, const std::string& name) {
        if (!transport.Connect(host, port)) return false;

        MsgHeader h{ MsgType::Hello, (uint32_t)name.size() };
        if (!transport.WriteExact(&h, sizeof(h))) return false;
        if (!name.empty() && !transport.WriteExact(name.data(), name.size())) return false;

        running = true;
        rx = std::thread(&NetClient::ReadLoop, this);
        return true;
    }

    void NetClient::Disconnect() {
        running = false;
        transport.Close();
        if (rx.joinable()) rx.join();
    }

    void NetClient::SendMoveCursor(float x, float y) {
        if (!running) return;
        PresenceMsg p{ 0, x, y };
        MsgHeader h{ MsgType::MoveCursor, sizeof(p) };
        std::lock_guard<std::mutex> lk(txMtx);
        transport.WriteExact(&h, sizeof(h));
        transport.WriteExact(&p, sizeof(p));
    }

    void NetClient::SendSetProperty(uint64_t entity, uint32_t comp,
        uint32_t version,
        const std::string& key,
        const std::string& value) {
        if (!running) return;
        SetPropertyMsg m{};
        m.entityID = entity;
        m.componentID = comp;
        m.version = version;
        m.keySize = (uint32_t)key.size();
        m.valueSize = (uint32_t)value.size();

        std::vector<char> buf(sizeof(m) + key.size() + value.size());
        std::memcpy(buf.data(), &m, sizeof(m));
        std::memcpy(buf.data() + sizeof(m), key.data(), key.size());
        std::memcpy(buf.data() + sizeof(m) + key.size(), value.data(), value.size());

        MsgHeader h{ MsgType::SetProperty, (uint32_t)buf.size() };
        std::lock_guard<std::mutex> lk(txMtx);
        transport.WriteExact(&h, sizeof(h));
        transport.WriteExact(buf.data(), buf.size());
    }

    void NetClient::ReadLoop() {
        while (running) {
            MsgHeader h;
            if (!transport.ReadExact(&h, sizeof(h))) break;
            std::vector<char> payload(h.size);
            if (h.size > 0 && !transport.ReadExact(payload.data(), h.size)) break;
            Handle(h.type, payload.data(), payload.size());
        }
        running = false;
    }

    void NetClient::Handle(MsgType t, const void* d, size_t n) {
        std::lock_guard<std::mutex> lk(mirror.mtx);
        switch (t) {
        case MsgType::Welcome: {
            if (n < sizeof(WelcomeMsg)) return;
            auto* w = (const WelcomeMsg*)d;
            mirror.localSessionID = w->sessionID;
            mirror.projectID = w->projectID;
            std::cout << "[NetClient] Welcome session=" << w->sessionID
                << " project=" << w->projectID << std::endl;
            break;
        }
        case MsgType::EntitySpawn: {
            if (n < sizeof(EntityMsg)) return;
            auto* e = (const EntityMsg*)d;
            mirror.components[e->entityID];   // ensure slot
            break;
        }
        case MsgType::PresenceUpdate: {
            if (n < sizeof(PresenceMsg)) return;
            auto* p = (const PresenceMsg*)d;
            mirror.presences[p->sessionID] = { p->cursorX, p->cursorY };
            break;
        }
        case MsgType::SnapshotDone:
            std::cout << "[NetClient] Snapshot complete" << std::endl;
            break;
        default: break;
        }
    }

} // namespace Net