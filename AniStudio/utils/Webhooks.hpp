// Webhooks.hpp
#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

// Alias for JSON library
using json = nlohmann::json;

// Define a type alias for the callback function
using WebhookCallback = std::function<void(const json& payload)>;

class Webhooks {
public:
    // Register a new webhook with a specific event
    void registerWebhook(const std::string& event, WebhookCallback callback);

    // Process an incoming webhook request
    void processRequest(const std::string& event, const json& payload);

    // Validate the webhook payload (you can customize this as needed)
    bool validateRequest(const std::string& secret, const std::string& payload, const std::string& signature);

private:
    // Map to store event names and their associated callback functions
    std::unordered_map<std::string, WebhookCallback> webhooks;
};

