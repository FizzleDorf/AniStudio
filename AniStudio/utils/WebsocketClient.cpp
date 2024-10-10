#include "WebSocketClient.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

WebSocketClient::WebSocketClient(const std::string& serverAddress, const std::string& clientId)
    : serverAddress(serverAddress), clientId(clientId) {
    // Initialize the WebSocket client
    wsClient.init_asio();
}

nlohmann::json WebSocketClient::queuePrompt(const std::string& prompt) {
    nlohmann::json requestPayload = {
        {"prompt", prompt},
        {"client_id", clientId}
    };

    std::string url = "http://" + serverAddress + "/prompt";
    std::string response;

    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestPayload.dump().c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* s) -> size_t {
            size_t newLength = size * nmemb;
            s->append((char*)contents, newLength);
            return newLength;
            });

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    return nlohmann::json::parse(response);
}

std::vector<std::string> WebSocketClient::getImages(const nlohmann::json& prompt) {
    std::vector<std::string> outputImages;
    std::string uri = "ws://" + serverAddress + "/ws?clientId=" + clientId;

    wsClient.set_message_handler([&outputImages](websocketpp::connection_hdl hdl, client::message_ptr msg) {
        // Parse the received message
        auto message = nlohmann::json::parse(msg->get_payload());

        if (message["type"] == "executing") {
            if (message["data"]["node"] == "save_image_websocket_node") {
                // Process image data
                outputImages.push_back(msg->get_payload().substr(8)); // Assume images start after the first 8 bytes
            }
        }
        });

    websocketpp::lib::error_code ec;
    client::connection_ptr con = wsClient.get_connection(uri, ec);

    if (ec) {
        std::cerr << "Connection Error: " << ec.message() << std::endl;
        return {};
    }

    wsClient.connect(con);
    wsClient.run();

    return outputImages;
}
