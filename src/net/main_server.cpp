#include "AniStudio.hpp"
#include "SocketTransport.hpp"
#include "SubscriptionRouter.hpp"
#include "ServerSession.hpp"
#include "Protocol.hpp"
#include "Discovery.hpp"
#include "ECS.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

using namespace Net;

struct ServerState {
    ANI::StudioCore* studioCore = nullptr;
    ECS::EntityManager* entityMgr = nullptr;
    SubscriptionRouter                                           router;
    std::mutex                                                   mtx;
    std::atomic<SessionID>                                       nextSession{ 1 };
    std::unordered_map<SessionID, std::shared_ptr<ServerSession>> sessions;
    const ProjectID                                              projectID = 1;
    std::string                                                  projectName = "AniStudio Project";
};

static void HandleCommand(SessionID sid, MsgType type,
    const void* data, size_t size,
    ServerState* st) {
    switch (type) {
    case MsgType::MoveCursor: {
        if (size < sizeof(PresenceMsg)) return;
        auto* p = (const PresenceMsg*)data;
        PresenceMsg out{ sid, p->cursorX, p->cursorY };
        st->router.PublishToProject(st->projectID, 1, &out, sizeof(out));
        break;
    }
    case MsgType::SetProperty: {
        st->router.PublishToProject(st->projectID, 0, data, size);
        break;
    }
    default: break;
    }
}

static void SendWelcome(ServerSession& s, ProjectID pid) {
    WelcomeMsg w{ s.ID(), pid };
    MsgHeader h{ MsgType::Welcome, sizeof(w) };
    s.SendRaw(&h, sizeof(h));
    s.SendRaw(&w, sizeof(w));
}

static void SendSnapshot(ServerSession& s, ServerState& st) {
    std::lock_guard<std::mutex> lk(st.mtx);
    if (!st.entityMgr) return;

    for (auto eid : st.entityMgr->GetAllEntities()) {
        EntityMsg em{ eid };
        MsgHeader h{ MsgType::EntitySpawn, sizeof(em) };
        s.SendRaw(&h, sizeof(h));
        s.SendRaw(&em, sizeof(em));
    }
    MsgHeader done{ MsgType::SnapshotDone, 0 };
    s.SendRaw(&done, sizeof(done));
}

int main(int argc, char** argv) {
    uint16_t port = 9000;
    if (argc > 1) port = (uint16_t)std::atoi(argv[1]);

    ANI::StudioCore serverCore;
    if (!serverCore.InitializeCoreOnly()) {
        std::cerr << "[Server] StudioCore::InitializeCoreOnly failed\n";
        return 1;
    }
    serverCore.SetMode(ANI::StudioContext::Mode::Server);

    ServerState state;
    state.studioCore = &serverCore;
    state.entityMgr = &serverCore.GetEntityManager();

    state.router.SetDeliver([&state](SessionID s, const void* payload, size_t n) {
        auto it = state.sessions.find(s);
        if (it == state.sessions.end()) return;
        it->second->SendRaw(payload, n);
        });

    SocketTransport listener;
    if (!listener.Listen(port)) {
        std::cerr << "[Server] Failed to listen on " << port << "\n";
        return 1;
    }
    std::cout << "[Server] Listening on " << port << "\n";

    // LAN discovery responder
    ServerInfo adv;
    adv.host = "AniStudio Server";
    adv.tcpPort = port;
    adv.projectName = state.projectName;
    adv.projectPath = "";
    adv.hostUser = "host";
    adv.clientCount = 0;
    adv.maxClients = 8;
    DiscoveryHandle* discovery = StartDiscoveryResponder(adv);

    auto last = std::chrono::steady_clock::now();

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listener.Fd(), &readfds);
        timeval tv{ 0, 16000 };
        int sel = select(listener.Fd() + 1, &readfds, nullptr, nullptr, &tv);

        if (sel > 0 && FD_ISSET(listener.Fd(), &readfds)) {
            int fd = listener.Accept();
            if (fd >= 0) {
                SessionID sid = state.nextSession++;

                auto sess = std::make_shared<ServerSession>(
                    fd, sid, state.projectID, state.router,
                    [&state](SessionID s, MsgType t, const void* d, size_t n) {
                        HandleCommand(s, t, d, n, &state);
                    });

                {
                    std::lock_guard<std::mutex> lk(state.mtx);
                    state.sessions[sid] = sess;
                }

                state.router.Subscribe(sid, { Scope::PerProject, state.projectID, 0, 0, 0 });
                state.router.Subscribe(sid, { Scope::PerProject, state.projectID, 0, 1, 0 });

                SendWelcome(*sess, state.projectID);
                SendSnapshot(*sess, state);
                sess->Start();

                std::cout << "[Server] Session " << sid << " connected\n";
            }
        }

        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        if (dt > 0.25f) dt = 0.25f;

        serverCore.Update(dt);
    }

    StopDiscoveryResponder(discovery);
    return 0;
}