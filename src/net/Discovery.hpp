#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Net {

    struct ServerInfo {
        std::string host;
        std::string address;
        uint16_t    tcpPort = 9000;
        std::string projectName;
        std::string projectPath;
        std::string hostUser;
        uint32_t    clientCount = 0;
        uint32_t    maxClients = 8;
    };

    struct DiscoveryHandle;
    DiscoveryHandle* StartDiscoveryResponder(const ServerInfo& info);
    void            StopDiscoveryResponder(DiscoveryHandle*);

    std::vector<ServerInfo> DiscoverServers(uint16_t discoveryPort = 9001,
        int      timeoutMs = 500);

} // namespace Net