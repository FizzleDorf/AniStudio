#ifndef WEBSOCKETCLIENT_HPP
#define WEBSOCKETCLIENT_HPP

#include <string>
#include <nlohmann/json.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

class WebSocketClient {
public:
    WebSocketClient(const std::string& serverAddress, const std::string& clientId);
    nlohmann::json queuePrompt(const std::string& prompt);
    std::vector<std::string> getImages(const nlohmann::json& prompt);

private:
    std::string serverAddress;
    std::string clientId;
    typedef websocketpp::client<websocketpp::config::asio_client> client;
    client wsClient;
};

#endif // WEBSOCKETCLIENT_HPP
