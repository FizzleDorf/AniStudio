#pragma once
#include "Protocol.hpp"
#include <mutex>
#include <unordered_map>
#include <string>
#include <vector>

namespace Net {

    // Client-side read-only mirror of the shared ECS.
    // Views read from this; they never touch the real EntityManager.
    class ClientMirror {
    public:
        struct Presence { float x = 0, y = 0; };

        std::mutex mtx;
        std::unordered_map<uint64_t, std::vector<uint8_t>> components; // key = entityID<<32 | compID
        std::unordered_map<SessionID, Presence> presences;
        SessionID localSessionID = 0;
        ProjectID projectID = 0;

        static uint64_t Key(uint64_t entityID, uint32_t compID) {
            return (entityID << 32) | compID;
        }
    };

} // namespace Net