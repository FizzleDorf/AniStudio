#pragma once
#include "ExampleComponent.hpp"
#include "ExampleSystem.hpp"
#include "ExampleView.hpp"

// Optional: Define any global plugin functionality here
namespace ExamplePlugin {
// Plugin version info
constexpr const char *PLUGIN_NAME = "ExamplePlugin";
constexpr const char *PLUGIN_VERSION = "1.0.0";

// Optional: Plugin configuration struct
struct PluginConfig {
    bool someOption = true;
    float someValue = 1.0f;
};

extern PluginConfig config;
} // namespace ExamplePlugin