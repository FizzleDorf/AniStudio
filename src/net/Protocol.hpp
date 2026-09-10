#pragma once
#include <cstdint>
#include <string>

namespace Net {

    using SessionID = uint64_t;
    using ProjectID = uint64_t;

    enum class MsgType : uint16_t {
        // server -> client
        Welcome = 1,
        EntitySpawn = 2,
        EntityDespawn = 3,
        ComponentSet = 4,
        PresenceUpdate = 5,
        SnapshotDone = 6,

        // client -> server
        Hello = 100,
        SetProperty = 101,
        MoveCursor = 102,
        AddComponent = 103,
        RemoveComponent = 104,
    };

#pragma pack(push, 1)
    struct MsgHeader {
        MsgType  type;
        uint32_t size;
    };

    struct WelcomeMsg {
        SessionID sessionID;
        ProjectID projectID;
    };

    struct EntityMsg {
        uint64_t entityID;
    };

    struct PresenceMsg {
        SessionID sessionID;
        float     cursorX;
        float     cursorY;
    };

    struct ComponentSetMsg {
        uint64_t entityID;
        uint32_t componentID;
        uint32_t version;
        uint32_t blobSize;   // followed by blob bytes
    };

    struct SetPropertyMsg {
        uint64_t entityID;
        uint32_t componentID;
        uint32_t version;
        uint32_t keySize;
        uint32_t valueSize;  // followed by key bytes then value bytes
    };
#pragma pack(pop)

} // namespace Net