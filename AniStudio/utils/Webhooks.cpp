// Webhooks.cpp

#include "Webhooks.hpp"
#include <iostream>
#include <openssl/hmac.h> // For request validation
#include <sstream>
#include <iomanip>

void Webhooks::registerWebhook(const std::string& event, WebhookCallback callback) {
    webhooks[event] = callback;
}

void Webhooks::processRequest(const std::string& event, const json& payload) {
    // Check if the webhook event is registered
    if (webhooks.find(event) != webhooks.end()) {
        // Call the registered callback function
        webhooks[event](payload);
    }
    else {
        std::cerr << "No webhook registered for event: " << event << std::endl;
    }
}

bool Webhooks::validateRequest(const std::string& secret, const std::string& payload, const std::string& signature) {
    unsigned char* result;
    unsigned int len = 20; // Adjust length as per hash function used

    // Create HMAC using SHA-256
    result = HMAC(EVP_sha256(), secret.c_str(), secret.length(), (unsigned char*)payload.c_str(), payload.length(), nullptr, nullptr);

    // Convert result to hex string
    std::stringstream ss;
    for (unsigned int i = 0; i < len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    }

    // Compare computed signature with provided signature
    return ss.str() == signature;
}

