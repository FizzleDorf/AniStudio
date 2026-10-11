#pragma once
#include "BaseComponent.hpp"
#include "PropertyTypes.hpp"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace ECS {

    class GeneralSettingsComponent : public BaseComponent {
    public:
        GeneralSettingsComponent() = default;

        const char* GetCompName() const override { return "GeneralSettings"; }
        const char* GetCompCategory() const override { return ""; }

        const nlohmann::json& GetSchema() const override {
            static const nlohmann::json j = {
                {"title", "General Settings"},
                {"type", "object"},
                {"propertyOrder", {
                    "showStartupScreen", "loadLastProject",
                    "autoSaveProjects", "autoSaveIntervalMinutes",
                    "confirmBeforeExit", "confirmBeforeDeleteAssets", "confirmBeforeOverwriteFiles",
                    "maxRecentProjects", "maxUndoLevels",
                    "enableHardwareAcceleration",
                    "enableLogging", "logLevel", "logToFile", "maxLogFileSize",
                    "useNewestPluginVersions"
                }},
                {"properties", {
                    {"showStartupScreen", {
                        {"type", "boolean"},
                        {"title", "Show Startup Screen"},
                        {"description", "Show the project manager on startup."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"loadLastProject", {
                        {"type", "boolean"},
                        {"title", "Load Last Project"},
                        {"description", "Automatically load the most recent project on startup."},
                        {"ui:widget", "checkbox"}
                    }},
                    {"autoSaveProjects", {
                        {"type", "boolean"},
                        {"title", "Auto-Save Projects"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"autoSaveIntervalMinutes", {
                        {"type", "integer"},
                        {"title", "Auto-Save Interval (min)"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"min", 1}, {"max", 60}}}
                    }},
                    {"confirmBeforeExit", {
                        {"type", "boolean"},
                        {"title", "Confirm Before Exit"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"confirmBeforeDeleteAssets", {
                        {"type", "boolean"},
                        {"title", "Confirm Before Delete Assets"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"confirmBeforeOverwriteFiles", {
                        {"type", "boolean"},
                        {"title", "Confirm Before Overwrite Files"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"maxRecentProjects", {
                        {"type", "integer"},
                        {"title", "Max Recent Projects"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"min", 5}, {"max", 50}}}
                    }},
                    {"maxUndoLevels", {
                        {"type", "integer"},
                        {"title", "Max Undo Levels"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"min", 10}, {"max", 1000}}}
                    }},
                    {"enableHardwareAcceleration", {
                        {"type", "boolean"},
                        {"title", "Enable Hardware Acceleration"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"enableLogging", {
                        {"type", "boolean"},
                        {"title", "Enable Logging"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"logLevel", {
                        {"type", "integer"},
                        {"title", "Log Level"},
                        {"description", "0=Error 1=Warning 2=Info 3=Debug"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"min", 0}, {"max", 3}}}
                    }},
                    {"logToFile", {
                        {"type", "boolean"},
                        {"title", "Log to File"},
                        {"ui:widget", "checkbox"}
                    }},
                    {"maxLogFileSize", {
                        {"type", "integer"},
                        {"title", "Max Log File Size (MB)"},
                        {"ui:widget", "input_int"},
                        {"ui:options", {{"step", 1}, {"min", 1}, {"max", 100}}}
                    }},
                    {"useNewestPluginVersions", {
                        {"type", "boolean"},
                        {"title", "Use Newest Plugin Versions"},
                        {"description", "Load newest plugin binaries from staging, overriding saved project versions."},
                        {"ui:widget", "checkbox"}
                    }}
                }}
            };
            return j;
        }

        std::unordered_map<std::string, Engine::PropertyVariant> GetPropertyMap() override {
            return {
                {"showStartupScreen", &showStartupScreen},
                {"loadLastProject", &loadLastProject},
                {"autoSaveProjects", &autoSaveProjects},
                {"autoSaveIntervalMinutes", &autoSaveIntervalMinutes},
                {"confirmBeforeExit", &confirmBeforeExit},
                {"confirmBeforeDeleteAssets", &confirmBeforeDeleteAssets},
                {"confirmBeforeOverwriteFiles", &confirmBeforeOverwriteFiles},
                {"maxRecentProjects", &maxRecentProjects},
                {"maxUndoLevels", &maxUndoLevels},
                {"enableHardwareAcceleration", &enableHardwareAcceleration},
                {"enableLogging", &enableLogging},
                {"logLevel", &logLevel},
                {"logToFile", &logToFile},
                {"maxLogFileSize", &maxLogFileSize},
                {"useNewestPluginVersions", &useNewestPluginVersions}
            };
        }

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& j) override;
        void ResetToDefaults();

        bool showStartupScreen = true;
        bool loadLastProject = true;
        bool autoSaveProjects = true;
        int autoSaveIntervalMinutes = 5;
        bool confirmBeforeExit = true;
        bool confirmBeforeDeleteAssets = true;
        bool confirmBeforeOverwriteFiles = true;
        int maxRecentProjects = 10;
        int maxUndoLevels = 100;
        bool enableHardwareAcceleration = true;
        bool enableLogging = true;
        int logLevel = 2;
        bool logToFile = true;
        int maxLogFileSize = 10;

        int windowWidth = 1200;
        int windowHeight = 720;
        int windowPosX = -1;
        int windowPosY = -1;
        bool windowMaximized = false;
        bool windowFullscreen = false;
        bool windowVsync = true;
        std::string lastOpenProject;
        std::vector<std::string> recentProjects;

        bool useNewestPluginVersions = false;
    };

}